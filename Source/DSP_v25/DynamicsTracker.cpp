#include "DynamicsTracker.h"

#include <cmath>

namespace audiosynth::dsp
{

float DynamicsTracker::microdynamicIndex(const FeatureFrame& f) const
{
    return juce::jlimit(0.0f, 1.0f, (f.crestDb - 3.0f) / 12.0f);
}

float DynamicsTracker::compressionCollapseRisk(const FeatureFrame& f, float truePeakDbTP) const
{
    const float crestPenalty = juce::jlimit(0.0f, 1.0f, (6.0f - f.crestDb) / 6.0f);
    const float tpRisk = juce::jlimit(0.0f, 1.0f, (-1.0f - truePeakDbTP) / -1.5f);
    return juce::jlimit(0.0f, 1.0f, 0.7f * crestPenalty + 0.3f * tpRisk);
}

} // namespace audiosynth::dsp
