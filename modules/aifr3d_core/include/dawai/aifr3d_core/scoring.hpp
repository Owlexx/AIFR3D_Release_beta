#pragma once

#include "dawai/aifr3d_core/feature_ingestion.hpp"

#include <string>
#include <vector>

namespace dawai::aifr3d_core
{

struct CategoryScore
{
    std::string category;
    double score = 0.0;
    double signalClarity = 1.0;
    // Deprecated alias retained for older serializers.
    double confidence = 1.0;
    double proBandLow = 0.0;
    double proBandHigh = 0.0;
};

struct ScoringResult
{
    std::vector<CategoryScore> categoryScores;
    double overallRating = 0.0;
};

class ScoringEngine
{
  public:
    [[nodiscard]] ScoringResult score(const FeatureSet& features) const;
};

} // namespace dawai::aifr3d_core
