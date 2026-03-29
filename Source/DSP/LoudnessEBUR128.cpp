#include "LoudnessEBUR128.h"

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>

namespace audiosynth::dsp
{

namespace
{
constexpr double kLoudnessFloor = 1.0e-12;
constexpr double kAbsoluteGateLufs = -70.0;
constexpr double kHighShelfFrequencyHz = 1681.974450955533;
constexpr double kHighShelfQ = 0.7071752369554196;
constexpr double kHighShelfGainDb = 4.0;
constexpr double kHighPassFrequencyHz = 38.13547087602444;
constexpr double kHighPassQ = 0.5003270373238773;

double energyToLufs(double energyPerSample)
{
    return -0.691 + 10.0 * std::log10(std::max(energyPerSample, kLoudnessFloor));
}

double linearToDb(double linear)
{
    return 20.0 * std::log10(std::max(linear, kLoudnessFloor));
}

LoudnessEBUR128::BiquadCoefficients makeHighShelf(double sampleRate, double frequencyHz,
                                                  double q, double gainDb)
{
    const double a = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * juce::MathConstants<double>::pi * frequencyHz / sampleRate;
    const double cosW0 = std::cos(w0);
    const double sinW0 = std::sin(w0);
    const double alpha = sinW0 / (2.0 * q);
    const double sqrtA = std::sqrt(a);

    const double b0 = a * ((a + 1.0) + (a - 1.0) * cosW0 + 2.0 * sqrtA * alpha);
    const double b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cosW0);
    const double b2 = a * ((a + 1.0) + (a - 1.0) * cosW0 - 2.0 * sqrtA * alpha);
    const double a0 = (a + 1.0) - (a - 1.0) * cosW0 + 2.0 * sqrtA * alpha;
    const double a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cosW0);
    const double a2 = (a + 1.0) - (a - 1.0) * cosW0 - 2.0 * sqrtA * alpha;

    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

LoudnessEBUR128::BiquadCoefficients makeHighPass(double sampleRate, double frequencyHz,
                                                 double q)
{
    const double w0 = 2.0 * juce::MathConstants<double>::pi * frequencyHz / sampleRate;
    const double cosW0 = std::cos(w0);
    const double sinW0 = std::sin(w0);
    const double alpha = sinW0 / (2.0 * q);

    const double b0 = (1.0 + cosW0) * 0.5;
    const double b1 = -(1.0 + cosW0);
    const double b2 = (1.0 + cosW0) * 0.5;
    const double a0 = 1.0 + alpha;
    const double a1 = -2.0 * cosW0;
    const double a2 = 1.0 - alpha;

    return {b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}
} // namespace

void LoudnessEBUR128::BiquadState::reset() noexcept
{
    x1 = x2 = y1 = y2 = 0.0;
}

float LoudnessEBUR128::BiquadState::process(float sample,
                                            const BiquadCoefficients& coefficients) noexcept
{
    const double x0 = static_cast<double>(sample);
    const double y0 = coefficients.b0 * x0 + coefficients.b1 * x1 + coefficients.b2 * x2 -
                      coefficients.a1 * y1 - coefficients.a2 * y2;

    x2 = x1;
    x1 = x0;
    y2 = y1;
    y1 = y0;
    return static_cast<float>(y0);
}

void LoudnessEBUR128::reset(double sampleRate, int maxBlockSize, int maxChannels)
{
    m_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    m_maxBlockSize = juce::jmax(64, maxBlockSize);
    m_stepSamples = juce::jmax(1, static_cast<int>(std::round(m_sampleRate * 0.1)));
    m_stepBlocks.clear();
    m_integratedBlocks.clear();
    m_stepAccumulatorEnergy = 0.0;
    m_stepAccumulatorSamples = 0;

    updateWeightingCoefficients();
    ensureChannelState(juce::jmax(1, maxChannels));

    const auto oversamplingChannels = static_cast<std::size_t>(juce::jmax(1, maxChannels));
    m_truePeakOversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        oversamplingChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, false);
    m_truePeakOversampler->reset();
    m_truePeakOversampler->initProcessing(static_cast<std::size_t>(m_maxBlockSize));
    m_truePeakScratch.setSize(static_cast<int>(oversamplingChannels), m_maxBlockSize, false, false,
                              true);
    m_truePeakScratch.clear();
}

void LoudnessEBUR128::updateWeightingCoefficients()
{
    m_shelfCoefficients =
        makeHighShelf(m_sampleRate, kHighShelfFrequencyHz, kHighShelfQ, kHighShelfGainDb);
    m_highPassCoefficients = makeHighPass(m_sampleRate, kHighPassFrequencyHz, kHighPassQ);
}

void LoudnessEBUR128::ensureChannelState(int channels)
{
    if (channels <= 0)
    {
        channels = 1;
    }

    if (static_cast<int>(m_shelfStates.size()) != channels)
    {
        m_shelfStates.assign(static_cast<std::size_t>(channels), {});
        m_highPassStates.assign(static_cast<std::size_t>(channels), {});
    }

    for (auto& state : m_shelfStates)
    {
        state.reset();
    }
    for (auto& state : m_highPassStates)
    {
        state.reset();
    }
}

double LoudnessEBUR128::computeTruePeakDbTp(const juce::AudioBuffer<float>& buffer)
{
    if (m_truePeakOversampler == nullptr || buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
    {
        return -120.0;
    }

    const int channels = juce::jmin(buffer.getNumChannels(), m_truePeakScratch.getNumChannels());
    const int samples = juce::jmin(buffer.getNumSamples(), m_truePeakScratch.getNumSamples());
    m_truePeakScratch.clear();

    for (int channel = 0; channel < channels; ++channel)
    {
        m_truePeakScratch.copyFrom(channel, 0, buffer, channel, 0, samples);
    }

    auto inputBlock = juce::dsp::AudioBlock<float>(m_truePeakScratch)
                          .getSubsetChannelBlock(0, static_cast<std::size_t>(m_truePeakScratch.getNumChannels()))
                          .getSubBlock(0, static_cast<std::size_t>(samples));
    const auto oversampled = m_truePeakOversampler->processSamplesUp(inputBlock);

    double truePeak = 0.0;
    for (std::size_t channel = 0; channel < oversampled.getNumChannels(); ++channel)
    {
        const auto* channelData = oversampled.getChannelPointer(channel);
        for (std::size_t sample = 0; sample < oversampled.getNumSamples(); ++sample)
        {
            truePeak = std::max(truePeak, std::abs(static_cast<double>(channelData[sample])));
        }
    }

    return linearToDb(truePeak);
}

float LoudnessEBUR128::computeWindowLufs(std::size_t steps) const
{
    double energy = m_stepAccumulatorEnergy;
    int samples = m_stepAccumulatorSamples;
    std::size_t collectedSteps = 0;

    for (auto it = m_stepBlocks.rbegin(); it != m_stepBlocks.rend() && collectedSteps < steps; ++it)
    {
        energy += it->sumSq;
        samples += it->samples;
        ++collectedSteps;
    }

    if (samples <= 0)
    {
        return -70.0f;
    }

    return static_cast<float>(energyToLufs(energy / static_cast<double>(samples)));
}

float LoudnessEBUR128::computeIntegratedLufs() const
{
    if (m_integratedBlocks.empty())
    {
        return -70.0f;
    }

    double absoluteGateEnergy = 0.0;
    int absoluteGateCount = 0;
    for (const auto& block : m_integratedBlocks)
    {
        if (block.loudnessLufs >= kAbsoluteGateLufs)
        {
            absoluteGateEnergy += block.energyPerSample;
            ++absoluteGateCount;
        }
    }

    if (absoluteGateCount <= 0)
    {
        return -70.0f;
    }

    const float ungatedLufs =
        static_cast<float>(energyToLufs(absoluteGateEnergy / static_cast<double>(absoluteGateCount)));
    const float relativeGate = ungatedLufs - 10.0f;

    double gatedEnergy = 0.0;
    int gatedCount = 0;
    for (const auto& block : m_integratedBlocks)
    {
        if (block.loudnessLufs >= kAbsoluteGateLufs && block.loudnessLufs >= relativeGate)
        {
            gatedEnergy += block.energyPerSample;
            ++gatedCount;
        }
    }

    if (gatedCount <= 0)
    {
        return ungatedLufs;
    }

    return static_cast<float>(energyToLufs(gatedEnergy / static_cast<double>(gatedCount)));
}

LoudnessFrame LoudnessEBUR128::process(const juce::AudioBuffer<float>& buffer)
{
    LoudnessFrame out;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
    {
        return out;
    }

    if (channels > static_cast<int>(m_shelfStates.size()))
    {
        ensureChannelState(channels);
    }

    double samplePeak = 0.0;
    for (int sample = 0; sample < samples; ++sample)
    {
        double weightedSampleEnergy = 0.0;
        for (int channel = 0; channel < channels; ++channel)
        {
            const float raw = buffer.getSample(channel, sample);
            samplePeak = std::max(samplePeak, std::abs(static_cast<double>(raw)));

            const float highPassed = m_highPassStates[static_cast<std::size_t>(channel)].process(
                raw, m_highPassCoefficients);
            const float filtered =
                m_shelfStates[static_cast<std::size_t>(channel)].process(highPassed,
                                                                         m_shelfCoefficients);
            weightedSampleEnergy += static_cast<double>(filtered * filtered);
        }

        m_stepAccumulatorEnergy += weightedSampleEnergy;
        ++m_stepAccumulatorSamples;

        if (m_stepAccumulatorSamples >= m_stepSamples)
        {
            m_stepBlocks.push_back({m_stepAccumulatorEnergy, m_stepAccumulatorSamples});
            if (m_stepBlocks.size() > 40)
            {
                m_stepBlocks.pop_front();
            }

            if (m_stepBlocks.size() >= 4)
            {
                double blockEnergy = 0.0;
                int blockSamples = 0;
                for (auto it = m_stepBlocks.rbegin(); it != m_stepBlocks.rbegin() + 4; ++it)
                {
                    blockEnergy += it->sumSq;
                    blockSamples += it->samples;
                }

                const double energyPerSample =
                    blockSamples > 0 ? (blockEnergy / static_cast<double>(blockSamples)) : 0.0;
                m_integratedBlocks.push_back(
                    {energyPerSample, static_cast<float>(energyToLufs(energyPerSample))});
            }

            m_stepAccumulatorEnergy = 0.0;
            m_stepAccumulatorSamples = 0;
        }
    }

    out.integratedLufs = computeIntegratedLufs();
    out.shortTermLufs = computeWindowLufs(30);
    out.momentaryLufs = computeWindowLufs(4);
    out.truePeakDbTP = static_cast<float>(computeTruePeakDbTp(buffer));
    out.peakDbFS = static_cast<float>(linearToDb(samplePeak));
    return out;
}

} // namespace audiosynth::dsp
