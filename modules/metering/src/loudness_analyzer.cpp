#include "dawai/metering/loudness_analyzer.hpp"

#include <algorithm>
#include <cmath>

namespace dawai::metering
{

namespace
{
constexpr double kEpsilon = 1.0e-12;

inline double toDb(double linear)
{
    return 20.0 * std::log10(std::max(linear, kEpsilon));
}

} // namespace

LoudnessAnalyzer::LoudnessAnalyzer(std::size_t shortTermSamples)
    : m_shortTermSamples(std::max<std::size_t>(shortTermSamples, 512))
{
}

void LoudnessAnalyzer::reset()
{
    m_integratedEnergy = 0.0;
    m_totalSamples = 0;
}

LoudnessMetrics LoudnessAnalyzer::analyze(const AudioBlock& block)
{
    LoudnessMetrics out;
    if (block.numChannels() == 0 || block.numSamples() == 0)
    {
        return out;
    }

    const std::size_t n = block.numSamples();
    const std::size_t channels = block.numChannels();

    double sumSquare = 0.0;
    double shortSquare = 0.0;
    double truePeak = 0.0;

    const std::size_t shortStart = (n > m_shortTermSamples) ? (n - m_shortTermSamples) : 0;

    for (std::size_t i = 0; i < n; ++i)
    {
        double mono = 0.0;
        for (std::size_t c = 0; c < channels; ++c)
        {
            mono += static_cast<double>(block.channels[c][i]);
            const double current = static_cast<double>(block.channels[c][i]);
            const double sampleAbs = std::abs(current);
            truePeak = std::max(truePeak, sampleAbs);

            if (i > 0)
            {
                const double prev = static_cast<double>(block.channels[c][i - 1]);
                truePeak = std::max(truePeak, std::abs((prev + current) * 0.5));
            }
        }

        mono /= static_cast<double>(channels);
        const double sq = mono * mono;
        sumSquare += sq;
        if (i >= shortStart)
        {
            shortSquare += sq;
        }
    }

    const double integratedPower = sumSquare / static_cast<double>(n);
    const double shortPower = shortSquare / static_cast<double>(n - shortStart);
    m_integratedEnergy += sumSquare;
    m_totalSamples += static_cast<std::uint64_t>(n);
    const double runningIntegratedPower =
        (m_totalSamples > 0) ? (m_integratedEnergy / static_cast<double>(m_totalSamples)) : 0.0;

    // Approximate LUFS conversion for deterministic relative comparisons.
    out.integratedLufs = -0.691 + 10.0 * std::log10(std::max(runningIntegratedPower, kEpsilon));
    out.shortTermLufs = -0.691 + 10.0 * std::log10(std::max(shortPower, kEpsilon));
    out.truePeakDbtp = toDb(truePeak);

    (void)integratedPower;
    return out;
}

DynamicMetrics DynamicAnalyzer::analyze(const AudioBlock& block) const
{
    DynamicMetrics out;
    if (block.numChannels() == 0 || block.numSamples() == 0)
    {
        return out;
    }

    const std::size_t n = block.numSamples();
    const std::size_t channels = block.numChannels();

    double sumSquare = 0.0;
    double peak = 0.0;
    std::size_t transientHits = 0;
    double previousMono = 0.0;
    bool hasPreviousMono = false;

    for (std::size_t i = 0; i < n; ++i)
    {
        double mono = 0.0;
        for (std::size_t c = 0; c < channels; ++c)
        {
            const double sample = static_cast<double>(block.channels[c][i]);
            mono += sample;
            peak = std::max(peak, std::abs(sample));
        }
        mono /= static_cast<double>(channels);
        sumSquare += mono * mono;
        if (hasPreviousMono && std::abs(mono - previousMono) > 0.08)
        {
            ++transientHits;
        }
        previousMono = mono;
        hasPreviousMono = true;
    }

    const double rms = std::sqrt(sumSquare / static_cast<double>(n));
    out.rmsDb = toDb(rms);
    out.peakDbfs = toDb(peak);
    out.crestFactorDb = toDb(peak) - out.rmsDb;
    out.transientDensity = std::clamp(static_cast<double>(transientHits) /
                                          static_cast<double>(std::max<std::size_t>(1, n - 1)),
                                      0.0, 1.0);
    return out;
}

} // namespace dawai::metering
