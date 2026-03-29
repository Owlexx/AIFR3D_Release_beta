#pragma once

#include "dawai/metering/metrics.hpp"

namespace dawai::metering
{

class StereoAnalyzer
{
  public:
    [[nodiscard]] StereoMetrics analyze(const AudioBlock& block) const;
};

} // namespace dawai::metering
