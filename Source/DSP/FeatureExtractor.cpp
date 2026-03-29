#include "FeatureExtractor.h"

#include <algorithm>
#include <cmath>

namespace aifred::dsp
{

namespace
{
constexpr float kEpsilon = 1.0e-9f;

float clamp01(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float dbFromEnergy(double energy)
{
    return 10.0f * std::log10(std::max(static_cast<double>(kEpsilon), energy));
}
} // namespace

void FeatureExtractor::prepare(double sampleRate, int maxBlockSize)
{
    m_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    m_maxBlockSize = maxBlockSize;
    m_processedSamples = 0.0;
    m_previousMidSample = 0.0f;
    m_fluxBaseline = 0.0;
    m_transientCooldownSamples = 0;
    m_transientEventTimesSec.clear();
}

FeatureFrame FeatureExtractor::extract(const juce::AudioBuffer<float>& buffer,
                                       const SpectralFrame* spectral)
{
    FeatureFrame out;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
    {
        return out;
    }

    double sumSqMid = 0.0;
    double sumSqSide = 0.0;
    double corrNum = 0.0;
    double corrDenL = 0.0;
    double corrDenR = 0.0;
    float peak = 0.0f;

    const double windowStartSec = m_processedSamples / m_sampleRate;
    const double windowEndSec = windowStartSec + (static_cast<double>(samples) / m_sampleRate);
    const int cooldownResetSamples = juce::jmax(1, static_cast<int>(std::round(m_sampleRate * 0.03)));

    for (int sample = 0; sample < samples; ++sample)
    {
        const float left = buffer.getSample(0, sample);
        const float right = (channels > 1) ? buffer.getSample(1, sample) : left;
        const float mid = 0.5f * (left + right);
        const float side = 0.5f * (left - right);

        sumSqMid += static_cast<double>(mid * mid);
        sumSqSide += static_cast<double>(side * side);
        peak = std::max(peak, std::abs(mid));

        corrNum += static_cast<double>(left * right);
        corrDenL += static_cast<double>(left * left);
        corrDenR += static_cast<double>(right * right);

        const double flux = static_cast<double>((mid - m_previousMidSample) * (mid - m_previousMidSample));
        m_fluxBaseline = (m_fluxBaseline * 0.985) + (flux * 0.015);
        const double fluxThreshold = std::max(1.0e-6, m_fluxBaseline * 8.0);

        if (m_transientCooldownSamples > 0)
        {
            --m_transientCooldownSamples;
        }

        if (flux > fluxThreshold && std::abs(mid) > 0.015f && m_transientCooldownSamples <= 0)
        {
            const double transientSec =
                windowStartSec + (static_cast<double>(sample) / m_sampleRate);
            m_transientEventTimesSec.push_back(transientSec);
            m_transientCooldownSamples = cooldownResetSamples;
        }

        m_previousMidSample = mid;
    }

    m_processedSamples += static_cast<double>(samples);

    while (!m_transientEventTimesSec.empty() &&
           m_transientEventTimesSec.front() < (windowEndSec - 1.0))
    {
        m_transientEventTimesSec.pop_front();
    }

    const float rms = std::sqrt(static_cast<float>(sumSqMid / static_cast<double>(samples)));
    out.rms = rms;
    out.peak = peak;
    out.crestDb = 20.0f * std::log10((peak + kEpsilon) / (rms + kEpsilon));

    const double stereoTotal = sumSqMid + sumSqSide + kEpsilon;
    out.midEnergy = static_cast<float>(sumSqMid / stereoTotal);
    out.sideEnergy = static_cast<float>(sumSqSide / stereoTotal);
    out.correlation = static_cast<float>(corrNum / (std::sqrt(corrDenL * corrDenR) + kEpsilon));
    out.correlation = juce::jlimit(-1.0f, 1.0f, out.correlation);

    const float transientWindowSec =
        static_cast<float>(std::max(0.1, std::min(1.0, windowEndSec)));
    out.transientRateHz =
        static_cast<float>(m_transientEventTimesSec.size()) / transientWindowSec;
    out.transientDensity = clamp01(out.transientRateHz / 12.0f);

    if (spectral != nullptr)
    {
        const float mudDelta =
            spectral->lowMidBandDb - (0.55f * spectral->lowBandDb + 0.45f * spectral->midBandDb);
        const float harshDelta =
            spectral->highMidBandDb - (0.55f * spectral->midBandDb + 0.45f * spectral->highBandDb);
        const float airDelta = spectral->airBandDb - spectral->highBandDb;

        out.mud = clamp01((mudDelta + 4.0f) / 12.0f);
        out.harshness = clamp01((harshDelta + 4.0f) / 12.0f);
        out.air = clamp01((airDelta + 6.0f) / 14.0f);
    }
    else
    {
        const float lowMidProxy = dbFromEnergy(sumSqMid / static_cast<double>(samples));
        const float highProxy = dbFromEnergy(sumSqSide / static_cast<double>(samples));
        out.mud = clamp01((lowMidProxy + 24.0f) / 24.0f);
        out.harshness = clamp01((highProxy + 24.0f) / 24.0f);
        out.air = clamp01((highProxy - lowMidProxy + 12.0f) / 24.0f);
    }

    return out;
}

} // namespace aifred::dsp
