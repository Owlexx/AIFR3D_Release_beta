#pragma once

#include "FeatureExtractor.h"

namespace audiosynth::dsp
{

class DynamicsTracker
{
  public:
    float microdynamicIndex(const FeatureFrame& f) const;
    float compressionCollapseRisk(const FeatureFrame& f, float truePeakDbTP) const;
};

} // namespace audiosynth::dsp
