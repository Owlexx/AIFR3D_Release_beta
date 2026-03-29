#pragma once

#include "dawai/metering/fft_analyzer.hpp"
#include "dawai/metering/loudness_analyzer.hpp"
#include "dawai/metering/stereo_analyzer.hpp"

namespace dawai::metering
{

class MeteringEngine
{
  public:
    MeteringEngine(std::size_t fftSize = 1024, std::size_t bandCount = 32,
                   std::size_t shortTermSamples = 48000 * 3);

    void reset();
    [[nodiscard]] MeterSnapshot process(const AudioBlock& block, double sampleRate);

  private:
    FFTAnalyzer m_fftAnalyzer;
    LoudnessAnalyzer m_loudnessAnalyzer;
    StereoAnalyzer m_stereoAnalyzer;
    DynamicAnalyzer m_dynamicAnalyzer;
};

} // namespace dawai::metering
