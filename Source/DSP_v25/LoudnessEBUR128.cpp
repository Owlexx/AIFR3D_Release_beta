#include "LoudnessEBUR128.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace aifred::dsp
{

namespace
{
constexpr double kLoudnessFloor = 1.0e-12;
constexpr double kMomentaryWindowMs = 400.0;
constexpr double kShortTermWindowMs = 3000.0;
constexpr double kIntegratedBlockMs = 400.0;
constexpr double kIntegratedHopMs = 100.0;
constexpr double kAbsoluteGateLufs = -70.0;
constexpr double kRelativeGateOffsetDb = 10.0;
constexpr double kKWeightShelfHz = 1681.974450955533;
constexpr double kKWeightShelfQ = 0.7071752369554196;
constexpr double kKWeightShelfGainDb = 4.0;
constexpr double kKWeightHighPassHz = 38.13547087602444;
constexpr double kKWeightHighPassQ = 0.5003270373238773;

int samplesFromMilliseconds(double sampleRate, double windowMs)
{
    return juce::jmax(1, static_cast<int>(std::llround(sampleRate * windowMs * 0.001)));
}

std::pair<double, int> accumulateRecentWindow(const std::deque<LoudnessEBUR128::EnergyBlock>& blocks,
                                              int targetSamples)
{
    double energy = 0.0;
    int samples = 0;

    for (auto it = blocks.rbegin(); it != blocks.rend() && samples < targetSamples; ++it)
    {
        const int remaining = targetSamples - samples;
        const int take = std::min(remaining, it->samples);
        if (take <= 0 || it->samples <= 0)
        {
            continue;
        }

        const double ratio = static_cast<double>(take) / static_cast<double>(it->samples);
        energy += it->sumSq * ratio;
        samples += take;
    }

    return {energy, samples};
}

template <typename T>
T maxAbs(T valueA, T valueB)
{
    return std::max(std::abs(valueA), std::abs(valueB));
}
} // namespace

void LoudnessEBUR128::reset(double sampleRate)
{
    m_sampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    m_recentBlocks.clear();
    m_recentSamples = 0;
    m_integratedBlockEnergy.clear();
    m_integratedBlockEnergy.reserve(4096);
    m_channelStates.assign(2, {});
    m_integratedHopSamplesPending = 0;
    m_lastProcessTimeSec = -1.0;
    m_recentChunkRetentionSamples = samplesFromMilliseconds(m_sampleRate, 3600.0);
    designKWeightingFilters();
}

void LoudnessEBUR128::designKWeightingFilters()
{
    const auto normalise = [](Biquad biquad, double a0)
    {
        biquad.b0 /= a0;
        biquad.b1 /= a0;
        biquad.b2 /= a0;
        biquad.a1 /= a0;
        biquad.a2 /= a0;
        return biquad;
    };

    const auto makeHighPass = [this, &normalise](double cutoffHz, double q)
    {
        const double w0 = juce::MathConstants<double>::twoPi * cutoffHz / m_sampleRate;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * q);

        Biquad biquad;
        const double a0 = 1.0 + alpha;
        biquad.b0 = (1.0 + cosW0) * 0.5;
        biquad.b1 = -(1.0 + cosW0);
        biquad.b2 = (1.0 + cosW0) * 0.5;
        biquad.a1 = -2.0 * cosW0;
        biquad.a2 = 1.0 - alpha;
        return normalise(biquad, a0);
    };

    const auto makeHighShelf = [this, &normalise](double cutoffHz, double q, double gainDb)
    {
        const double a = std::pow(10.0, gainDb / 40.0);
        const double w0 = juce::MathConstants<double>::twoPi * cutoffHz / m_sampleRate;
        const double cosW0 = std::cos(w0);
        const double sinW0 = std::sin(w0);
        const double alpha = sinW0 / (2.0 * q);
        const double beta = 2.0 * std::sqrt(a) * alpha;

        Biquad biquad;
        const double a0 = (a + 1.0) - ((a - 1.0) * cosW0) + beta;
        biquad.b0 = a * ((a + 1.0) + ((a - 1.0) * cosW0) + beta);
        biquad.b1 = -2.0 * a * ((a - 1.0) + ((a + 1.0) * cosW0));
        biquad.b2 = a * ((a + 1.0) + ((a - 1.0) * cosW0) - beta);
        biquad.a1 = 2.0 * ((a - 1.0) - ((a + 1.0) * cosW0));
        biquad.a2 = (a + 1.0) - ((a - 1.0) * cosW0) - beta;
        return normalise(biquad, a0);
    };

    for (auto& state : m_channelStates)
    {
        state.shelf = makeHighShelf(kKWeightShelfHz, kKWeightShelfQ, kKWeightShelfGainDb);
        state.highPass = makeHighPass(kKWeightHighPassHz, kKWeightHighPassQ);
        state.shelf.reset();
        state.highPass.reset();
    }
}

double LoudnessEBUR128::filteredEnergyForWindow(int targetSamples) const
{
    return accumulateRecentWindow(m_recentBlocks, targetSamples).first;
}

float LoudnessEBUR128::loudnessFromEnergy(double energyPerSample) const
{
    return static_cast<float>(-0.691 + 10.0 * std::log10(std::max(energyPerSample, kLoudnessFloor)));
}

float LoudnessEBUR128::computeIntegratedLufs() const
{
    if (m_integratedBlockEnergy.empty())
    {
        return static_cast<float>(kAbsoluteGateLufs);
    }

    std::vector<double> absoluteGated;
    absoluteGated.reserve(m_integratedBlockEnergy.size());
    for (const auto blockEnergy : m_integratedBlockEnergy)
    {
        if (loudnessFromEnergy(blockEnergy) >= static_cast<float>(kAbsoluteGateLufs))
        {
            absoluteGated.push_back(blockEnergy);
        }
    }

    if (absoluteGated.empty())
    {
        return static_cast<float>(kAbsoluteGateLufs);
    }

    const double preliminaryEnergy =
        std::accumulate(absoluteGated.begin(), absoluteGated.end(), 0.0) /
        static_cast<double>(absoluteGated.size());
    const float preliminaryLufs = loudnessFromEnergy(preliminaryEnergy);
    const float relativeGate = preliminaryLufs - static_cast<float>(kRelativeGateOffsetDb);

    std::vector<double> relativeGated;
    relativeGated.reserve(absoluteGated.size());
    for (const auto blockEnergy : absoluteGated)
    {
        if (loudnessFromEnergy(blockEnergy) >= relativeGate)
        {
            relativeGated.push_back(blockEnergy);
        }
    }

    if (relativeGated.empty())
    {
        return preliminaryLufs;
    }

    const double finalEnergy =
        std::accumulate(relativeGated.begin(), relativeGated.end(), 0.0) /
        static_cast<double>(relativeGated.size());
    return loudnessFromEnergy(finalEnergy);
}

float LoudnessEBUR128::computeTruePeakDbTP(const juce::AudioBuffer<float>& buffer) const
{
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
    {
        return -120.0f;
    }

    const int oversampleFactor = m_sampleRate < 96000.0 ? 4 : 1;
    float maxPeak = 0.0f;

    for (int channel = 0; channel < channels; ++channel)
    {
        const auto* data = buffer.getReadPointer(channel);
        for (int sample = 0; sample < samples; ++sample)
        {
            maxPeak = std::max(maxPeak, std::abs(data[sample]));
        }

        if (oversampleFactor <= 1)
        {
            continue;
        }

        for (int sample = 0; sample < samples - 1; ++sample)
        {
            const float current = data[sample];
            const float next = data[sample + 1];
            for (int step = 1; step < oversampleFactor; ++step)
            {
                const float t = static_cast<float>(step) / static_cast<float>(oversampleFactor);
                const float interpolated = current + ((next - current) * t);
                maxPeak = std::max(maxPeak, std::abs(interpolated));
            }
        }
    }

    return static_cast<float>(20.0 * std::log10(std::max(static_cast<double>(maxPeak), 1.0e-12)));
}

LoudnessFrame LoudnessEBUR128::process(const juce::AudioBuffer<float>& buffer, double tSec)
{
    LoudnessFrame out;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
    {
        return out;
    }

    if (static_cast<int>(m_channelStates.size()) < channels)
    {
        m_channelStates.resize(static_cast<std::size_t>(channels));
        designKWeightingFilters();
    }

    int startSample = 0;
    if (m_lastProcessTimeSec >= 0.0 && tSec >= m_lastProcessTimeSec)
    {
        const int newSamples =
            juce::jlimit(0, samples,
                         static_cast<int>(std::llround((tSec - m_lastProcessTimeSec) * m_sampleRate)));
        if (newSamples > 0)
        {
            startSample = samples - newSamples;
        }
        else
        {
            startSample = samples;
        }
    }

    const int processedSamples = samples - startSample;
    double newEnergy = 0.0;

    if (processedSamples > 0)
    {
        for (int sample = startSample; sample < samples; ++sample)
        {
            double weightedSampleEnergy = 0.0;
            for (int channel = 0; channel < channels; ++channel)
            {
                const double input = static_cast<double>(buffer.getSample(channel, sample));
                auto& state = m_channelStates[static_cast<std::size_t>(channel)];
                const double shelf = state.shelf.process(input);
                const double filtered = state.highPass.process(shelf);
                weightedSampleEnergy += filtered * filtered;
            }
            newEnergy += weightedSampleEnergy;
        }

        m_recentBlocks.push_back({newEnergy, processedSamples});
        m_recentSamples += processedSamples;
        m_integratedHopSamplesPending += processedSamples;

        while (m_recentSamples > m_recentChunkRetentionSamples && !m_recentBlocks.empty())
        {
            m_recentSamples -= m_recentBlocks.front().samples;
            m_recentBlocks.pop_front();
        }

        const int integratedBlockSamples = samplesFromMilliseconds(m_sampleRate, kIntegratedBlockMs);
        const int integratedHopSamples = samplesFromMilliseconds(m_sampleRate, kIntegratedHopMs);
        while (m_integratedHopSamplesPending >= integratedHopSamples)
        {
            if (m_recentSamples >= integratedBlockSamples)
            {
                const auto [blockEnergy, blockSamples] =
                    accumulateRecentWindow(m_recentBlocks, integratedBlockSamples);
                if (blockSamples > 0)
                {
                    m_integratedBlockEnergy.push_back(blockEnergy / static_cast<double>(blockSamples));
                }
            }
            m_integratedHopSamplesPending -= integratedHopSamples;
        }
    }

    const int momentarySamples = samplesFromMilliseconds(m_sampleRate, kMomentaryWindowMs);
    const int shortTermSamples = samplesFromMilliseconds(m_sampleRate, kShortTermWindowMs);
    const auto [momentaryEnergy, momentaryCount] =
        accumulateRecentWindow(m_recentBlocks, momentarySamples);
    const auto [shortTermEnergy, shortTermCount] =
        accumulateRecentWindow(m_recentBlocks, shortTermSamples);

    out.momentaryLufs = loudnessFromEnergy(momentaryEnergy /
                                           static_cast<double>(juce::jmax(1, momentaryCount)));
    out.shortTermLufs =
        loudnessFromEnergy(shortTermEnergy / static_cast<double>(juce::jmax(1, shortTermCount)));
    out.integratedLufs = computeIntegratedLufs();
    out.truePeakDbTP = computeTruePeakDbTP(buffer);

    m_lastProcessTimeSec = tSec;
    return out;
}

} // namespace aifred::dsp
