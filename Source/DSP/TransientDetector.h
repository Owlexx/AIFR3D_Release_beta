#pragma once

#include "FeatureExtractor.h"

namespace aifred::dsp
{

class TransientDetector
{
  public:
    float transientMatch(const FeatureFrame& user, const FeatureFrame& reference) const;
};

} // namespace aifred::dsp
