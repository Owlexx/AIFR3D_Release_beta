#pragma once

#include "dawai/metering/metrics.hpp"

#include <cstddef>

namespace dawai::metering
{

class FFTAnalyzer
{
  public:
    explicit FFTAnalyzer(std::size_t fftSize = 1024, std::size_t bandCount = 32);

    [[nodiscard]] SpectrumMetrics analyze(const AudioBlock& block, double sampleRate) const;

  private:
    std::size_t m_fftSize;
    std::size_t m_bandCount;
};

} // namespace dawai::metering
