#pragma once

#include "dawai/metering/metrics.hpp"

#include <cstddef>
#include <cstdint>

namespace dawai::metering
{

class LoudnessAnalyzer
{
  public:
    explicit LoudnessAnalyzer(std::size_t shortTermSamples = 48000 * 3);

    void reset();
    [[nodiscard]] LoudnessMetrics analyze(const AudioBlock& block);

  private:
    std::size_t m_shortTermSamples;
    double m_integratedEnergy = 0.0;
    std::uint64_t m_totalSamples = 0;
};

class DynamicAnalyzer
{
  public:
    [[nodiscard]] DynamicMetrics analyze(const AudioBlock& block) const;
};

} // namespace dawai::metering
