#pragma once

#include "FeatureExtractor.h"

namespace audiosynth::dsp
{

class StereoAnalyzer
{
  public:
    float stereoSignatureMatch(const FeatureFrame& user, const FeatureFrame& reference) const;
    float phaseRisk(const FeatureFrame& f) const;
    float subMonoIntegrity(const FeatureFrame& f) const;
};

} // namespace audiosynth::dsp
