#pragma once

#include "dawai/aifr3d_core/scoring.hpp"

#include <string>
#include <vector>

namespace dawai::aifr3d_core
{

struct FixListItem
{
    int priority = 0;
    std::string category;
    std::string whatChanged;
    std::string whereYouStand;
    std::string whatToDoNext;
    std::vector<std::string> options;
    double signalClarity = 1.0;
    // Deprecated alias retained for older serializers.
    double confidence = 1.0;
    double score = 0.0;
    double targetSeverity = 0.0;
    double referenceSeverity = 0.0;
    double targetDelta = 0.0;
    double referenceDelta = 0.0;
};

class FixListGenerator
{
  public:
    [[nodiscard]] std::vector<FixListItem> generate(const FeatureSet& features,
                                                    const ScoringResult& scoring) const;
};

} // namespace dawai::aifr3d_core
