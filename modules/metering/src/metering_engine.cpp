#include "dawai/metering/metering_engine.hpp"

#include <algorithm>

namespace dawai::metering
{

std::size_t AudioBlock::numSamples() const noexcept
{
    if (channels.empty())
    {
        return 0;
    }

    std::size_t minSamples = channels.front().size();
    for (const auto& channel : channels)
    {
        minSamples = std::min(minSamples, channel.size());
    }
    return minSamples;
}

MeteringEngine::MeteringEngine(std::size_t fftSize, std::size_t bandCount,
                               std::size_t shortTermSamples)
    : m_fftAnalyzer(fftSize, bandCount), m_loudnessAnalyzer(shortTermSamples)
{
}

void MeteringEngine::reset()
{
    m_loudnessAnalyzer.reset();
}

MeterSnapshot MeteringEngine::process(const AudioBlock& block, double sampleRate)
{
    MeterSnapshot out;
    out.spectrum = m_fftAnalyzer.analyze(block, sampleRate);
    out.loudness = m_loudnessAnalyzer.analyze(block);
    out.stereo = m_stereoAnalyzer.analyze(block);
    out.dynamics = m_dynamicAnalyzer.analyze(block);
    return out;
}

} // namespace dawai::metering
