#include "StereoAnalyzer.h"

#include <cmath>

namespace aifred::dsp
{

float StereoAnalyzer::stereoSignatureMatch(const FeatureFrame& user,
                                           const FeatureFrame& reference) const
{
    const float midDiff = std::abs(user.midEnergy - reference.midEnergy);
    const float sideDiff = std::abs(user.sideEnergy - reference.sideEnergy);
    const float corrDiff = std::abs(user.correlation - reference.correlation) * 0.5f;
    const float d = std::sqrt(midDiff * midDiff + sideDiff * sideDiff + corrDiff * corrDiff);
    return std::exp(-1.2f * d);
}

float StereoAnalyzer::phaseRisk(const FeatureFrame& f) const
{
    return juce::jlimit(0.0f, 1.0f, (0.25f - f.correlation) / 0.55f);
}

float StereoAnalyzer::subMonoIntegrity(const FeatureFrame& f) const
{
    return juce::jlimit(0.0f, 1.0f, f.midEnergy / (f.midEnergy + f.sideEnergy + 1.0e-6f));
}

} // namespace aifred::dsp
