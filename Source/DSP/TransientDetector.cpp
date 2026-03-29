#include "TransientDetector.h"

#include <cmath>

namespace aifred::dsp
{

float TransientDetector::transientMatch(const FeatureFrame& user,
                                        const FeatureFrame& reference) const
{
    const float densityDiff = std::abs(user.transientDensity - reference.transientDensity);
    const float crestDiff = std::abs(user.crestDb - reference.crestDb) / 12.0f;
    const float d = std::sqrt(densityDiff * densityDiff + crestDiff * crestDiff);
    return std::exp(-0.9f * d);
}

} // namespace aifred::dsp
