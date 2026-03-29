#pragma once

#include "FeatureExtractor.h"

namespace audiosynth::dsp
{

class TransientDetector
{
  public:
    float transientMatch(const FeatureFrame& user, const FeatureFrame& reference) const;
};

} // namespace audiosynth::dsp
