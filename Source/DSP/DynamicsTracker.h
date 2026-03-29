#pragma once

#include "FeatureExtractor.h"

namespace aifred::dsp
{

class DynamicsTracker
{
  public:
    float microdynamicIndex(const FeatureFrame& f) const;
    float compressionCollapseRisk(const FeatureFrame& f, float truePeakDbTP) const;
};

} // namespace aifred::dsp
