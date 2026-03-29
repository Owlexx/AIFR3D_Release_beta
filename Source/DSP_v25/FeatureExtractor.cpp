#include "FeatureExtractor.h"

#include <algorithm>
#include <cmath>

namespace audiosynth::dsp
{

namespace
{
constexpr float kEpsilon = 1.0e-9f;

float clampDb(float energy)
{
    return 10.0f * std::log10(std::max(kEpsilon, energy));
}
} // namespace

void FeatureExtractor::prepare(double sampleRate, int maxBlockSize)
{
    m_sampleRate = sampleRate;
    m_maxBlockSize = maxBlockSize;
    (void)m_maxBlockSize;
}

FeatureFrame FeatureExtractor::extract(const juce::AudioBuffer<float>& buffer,
                                       const SpectralFrame* spectral) const
{
    FeatureFrame out;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return out;

    double sumSqAllChannels = 0.0;
    double sumSqMid = 0.0;
    float peak = 0.0f;
    double sumSqSide = 0.0;
    double corrNum = 0.0;
    double corrDenL = 0.0;
    double corrDenR = 0.0;
    int transientCount = 0;

    float prevMidAbs = 0.0f;

    for (int i = 0; i < samples; ++i)
    {
        const float l = buffer.getSample(0, i);
        const float r = (channels > 1) ? buffer.getSample(1, i) : l;

        const float mid = 0.5f * (l + r);
        const float side = 0.5f * (l - r);

        sumSqAllChannels += static_cast<double>(l * l);
        sumSqAllChannels += static_cast<double>(r * r);
        sumSqMid += static_cast<double>(mid * mid);
        sumSqSide += static_cast<double>(side * side);
        peak = std::max(peak, std::abs(l));
        peak = std::max(peak, std::abs(r));

        corrNum += static_cast<double>(l * r);
        corrDenL += static_cast<double>(l * l);
        corrDenR += static_cast<double>(r * r);

        const float midAbs = std::abs(mid);
        if (midAbs > prevMidAbs * 1.8f && midAbs > 0.03f)
            ++transientCount;
        prevMidAbs = std::max(midAbs, prevMidAbs * 0.98f);
    }

    const double rmsDenominator =
        static_cast<double>(samples * juce::jmax(1, channels == 1 ? 1 : 2));
    const float rms = std::sqrt(static_cast<float>(sumSqAllChannels / rmsDenominator));
    out.rms = rms;
    out.peak = peak;
    out.crestDb = 20.0f * std::log10((peak + kEpsilon) / (rms + kEpsilon));

    const double total = sumSqMid + sumSqSide + kEpsilon;
    out.midEnergy = static_cast<float>(sumSqMid / total);
    out.sideEnergy = static_cast<float>(sumSqSide / total);
    out.correlation = static_cast<float>(corrNum / (std::sqrt(corrDenL * corrDenR) + kEpsilon));
    out.correlation = juce::jlimit(-1.0f, 1.0f, out.correlation);

    out.transientDensity = juce::jlimit(0.0f, 1.0f,
                                        static_cast<float>(transientCount) /
                                            static_cast<float>(std::max(samples / 64, 1)));

    // Lightweight realtime-safe band proxies from cascaded one-pole responses.
    const float dt = 1.0f / static_cast<float>(std::max(1.0, m_sampleRate));
    const auto alphaFor = [dt](float hz)
    {
        const float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * hz);
        return dt / (rc + dt);
    };

    const float alphaLow = alphaFor(160.0f);   // body
    const float alphaMid = alphaFor(1200.0f);  // control mid
    const float alphaHigh = alphaFor(5000.0f); // harsh band proxy
    const float alphaAir = alphaFor(11000.0f); // air proxy

    float lowState = 0.0f;
    float midState = 0.0f;
    float highState = 0.0f;
    float airState = 0.0f;
    double lowEnergy = 0.0;
    double mudEnergy = 0.0;
    double controlEnergy = 0.0;
    double harshEnergy = 0.0;
    double airEnergy = 0.0;

    for (int i = 0; i < samples; ++i)
    {
        const float l = buffer.getSample(0, i);
        const float r = (channels > 1) ? buffer.getSample(1, i) : l;
        const float mid = 0.5f * (l + r);

        lowState += alphaLow * (mid - lowState);
        midState += alphaMid * (mid - midState);
        highState += alphaHigh * (mid - highState);
        airState += alphaAir * (mid - airState);

        const float bodyBand = lowState;
        const float mudBand = midState - lowState;
        const float controlBand = highState - midState;
        const float harshBand = mid - highState;
        const float airBand = harshBand - (mid - airState);

        lowEnergy += static_cast<double>(bodyBand * bodyBand);
        mudEnergy += static_cast<double>(mudBand * mudBand);
        controlEnergy += static_cast<double>(controlBand * controlBand);
        harshEnergy += static_cast<double>(harshBand * harshBand);
        airEnergy += static_cast<double>(airBand * airBand);
    }

    const float eBody = clampDb(static_cast<float>(lowEnergy / static_cast<double>(samples)));
    const float eMud = clampDb(static_cast<float>(mudEnergy / static_cast<double>(samples)));
    const float eControl =
        clampDb(static_cast<float>(controlEnergy / static_cast<double>(samples)));
    const float eHarsh = clampDb(static_cast<float>(harshEnergy / static_cast<double>(samples)));
    const float eAir = clampDb(static_cast<float>(airEnergy / static_cast<double>(samples)));

    const float harshRatio = eHarsh - eControl;
    const float mudRatio = eMud - eBody;
    const float airRatio = eAir - eControl;

    out.harshness = juce::jlimit(0.0f, 1.0f, (harshRatio + 6.0f) / 18.0f);
    out.mud = juce::jlimit(0.0f, 1.0f, (mudRatio + 6.0f) / 18.0f);
    out.air = juce::jlimit(0.0f, 1.0f, (airRatio + 6.0f) / 18.0f);

    if (spectral != nullptr)
    {
        const float spectralMud =
            juce::jlimit(0.0f, 1.0f, (spectral->lowMidBandDb - spectral->midBandDb + 6.0f) / 18.0f);
        const float spectralHarshness = juce::jlimit(
            0.0f, 1.0f, (spectral->highMidBandDb - spectral->midBandDb + 6.0f) / 18.0f);
        const float spectralAir =
            juce::jlimit(0.0f, 1.0f, (spectral->highBandDb - spectral->highMidBandDb + 6.0f) / 18.0f);

        out.mud = spectralMud;
        out.harshness = spectralHarshness;
        out.air = spectralAir;
    }

    return out;
}

} // namespace audiosynth::dsp
