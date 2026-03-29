#pragma once

#include "FeatureExtractor.h"

namespace aifred::dsp
{

class StereoAnalyzer
{
  public:
    float stereoSignatureMatch(const FeatureFrame& user, const FeatureFrame& reference) const;
    float phaseRisk(const FeatureFrame& f) const;
    float subMonoIntegrity(const FeatureFrame& f) const;
};

} // namespace aifred::dsp
