#include "StereoAnalyzer.h"

#include <cmath>

namespace audiosynth::dsp
{

namespace
{
constexpr float kEpsilon = 1.0e-9f;

float clampUnit(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}
} // namespace

void StereoAnalyzer::prepare(double sampleRate) noexcept
{
    m_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    m_previousField = StereoFieldFrame{};
    m_hasPreviousField = false;
}

StereoFieldFrame StereoAnalyzer::analyze(const juce::AudioBuffer<float>& buffer)
{
    StereoFieldFrame out;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
    {
        return out;
    }

    const float dt = 1.0f / static_cast<float>(juce::jmax(1.0, m_sampleRate));
    const float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * 120.0f);
    const float alphaLow = dt / (rc + dt);

    double sumSqL = 0.0;
    double sumSqR = 0.0;
    double sumSqMid = 0.0;
    double sumSqSide = 0.0;
    double corrNum = 0.0;
    double corrDenL = 0.0;
    double corrDenR = 0.0;

    float lowL = 0.0f;
    float lowR = 0.0f;
    double lowCorrNum = 0.0;
    double lowCorrDenL = 0.0;
    double lowCorrDenR = 0.0;
    double lowMidEnergy = 0.0;
    double lowSideEnergy = 0.0;

    for (int i = 0; i < samples; ++i)
    {
        const float l = buffer.getSample(0, i);
        const float r = (channels > 1) ? buffer.getSample(1, i) : l;

        const float mid = 0.5f * (l + r);
        const float side = 0.5f * (l - r);

        sumSqL += static_cast<double>(l * l);
        sumSqR += static_cast<double>(r * r);
        sumSqMid += static_cast<double>(mid * mid);
        sumSqSide += static_cast<double>(side * side);

        corrNum += static_cast<double>(l * r);
        corrDenL += static_cast<double>(l * l);
        corrDenR += static_cast<double>(r * r);

        lowL += alphaLow * (l - lowL);
        lowR += alphaLow * (r - lowR);
        const float lowMid = 0.5f * (lowL + lowR);
        const float lowSide = 0.5f * (lowL - lowR);
        lowMidEnergy += static_cast<double>(lowMid * lowMid);
        lowSideEnergy += static_cast<double>(lowSide * lowSide);
        lowCorrNum += static_cast<double>(lowL * lowR);
        lowCorrDenL += static_cast<double>(lowL * lowL);
        lowCorrDenR += static_cast<double>(lowR * lowR);
    }

    const double stereoTotal = sumSqMid + sumSqSide + kEpsilon;
    const double lrTotal = sumSqL + sumSqR + kEpsilon;
    out.leftEnergy = clampUnit(static_cast<float>(sumSqL / lrTotal));
    out.rightEnergy = clampUnit(static_cast<float>(sumSqR / lrTotal));
    out.midEnergy = clampUnit(static_cast<float>(sumSqMid / stereoTotal));
    out.sideEnergy = clampUnit(static_cast<float>(sumSqSide / stereoTotal));
    out.width = clampUnit(static_cast<float>(sumSqSide / stereoTotal));
    out.correlation = juce::jlimit(
        -1.0f, 1.0f,
        static_cast<float>(corrNum / (std::sqrt(corrDenL * corrDenR) + kEpsilon)));

    const double lowTotal = lowMidEnergy + lowSideEnergy + kEpsilon;
    const float lowSideRatio = clampUnit(static_cast<float>(lowSideEnergy / lowTotal));
    out.subMonoIntegrity = clampUnit(1.0f - lowSideRatio);
    out.lowBandCorrelation = juce::jlimit(
        -1.0f, 1.0f,
        static_cast<float>(lowCorrNum / (std::sqrt(lowCorrDenL * lowCorrDenR) + kEpsilon)));
    out.lowBandPhaseRisk = clampUnit(juce::jmax(0.0f, -out.lowBandCorrelation));

    const float balanceSkew = std::abs(out.leftEnergy - out.rightEnergy);
    out.centerDominance = clampUnit(0.74f * out.midEnergy + 0.26f * (1.0f - balanceSkew));
    out.sideDominance = clampUnit(out.sideEnergy);
    out.phaseRisk = juce::jmax(clampUnit(juce::jmax(0.0f, -out.correlation)),
                               out.lowBandPhaseRisk);

    const float corrDecorrelation = clampUnit(1.0f - (0.5f * (out.correlation + 1.0f)));
    if (m_hasPreviousField)
    {
        const float movement =
            std::abs(out.leftEnergy - m_previousField.leftEnergy) +
            std::abs(out.rightEnergy - m_previousField.rightEnergy) +
            std::abs(out.midEnergy - m_previousField.midEnergy) +
            std::abs(out.sideEnergy - m_previousField.sideEnergy);
        out.stereoMotion = clampUnit(movement * 2.5f);
    }
    else
    {
        out.stereoMotion = 0.0f;
    }
    out.spatialSpread =
        clampUnit(0.58f * out.width + 0.24f * corrDecorrelation + 0.18f * out.stereoMotion);

    out.wAxis = out.midEnergy;
    out.xAxis = out.leftEnergy;
    out.yAxis = out.rightEnergy;
    out.zAxis = out.sideEnergy;

    m_previousField = out;
    m_hasPreviousField = true;
    return out;
}

float StereoAnalyzer::stereoSignatureMatch(const FeatureFrame& user,
                                           const FeatureFrame& reference) const
{
    const float midDiff = std::abs(user.midEnergy - reference.midEnergy);
    const float sideDiff = std::abs(user.sideEnergy - reference.sideEnergy);
    const float corrDiff = std::abs(user.correlation - reference.correlation) * 0.5f;
    const float d = std::sqrt(midDiff * midDiff + sideDiff * sideDiff + corrDiff * corrDiff);
    return std::exp(-1.2f * d);
}

float StereoAnalyzer::phaseRisk(const FeatureFrame& f) const
{
    return juce::jlimit(0.0f, 1.0f, (0.25f - f.correlation) / 0.55f);
}

float StereoAnalyzer::subMonoIntegrity(const FeatureFrame& f) const
{
    return juce::jlimit(0.0f, 1.0f, f.midEnergy / (f.midEnergy + f.sideEnergy + 1.0e-6f));
}

} // namespace audiosynth::dsp
