#include "dawai/aifr3d_core/scoring.hpp"

#include <algorithm>
#include <cmath>

namespace dawai::aifr3d_core
{

ScoringResult ScoringEngine::score(const FeatureSet& features) const
{
    ScoringResult result;

    double scoreSum = 0.0;
    for (const auto& deviation : features.categoryDeviations)
    {
        CategoryScore score;
        score.category = deviation.category;

        const double targetWindow = std::max(features.targetWindow, 0.25);
        const double targetRatio = std::max(0.0, deviation.deviation / targetWindow);
        const double referenceRatio = std::max(0.0, deviation.referenceDeviation / targetWindow);

        double penalty = 0.0;
        if (targetRatio <= 1.0)
        {
            penalty = 0.40 * std::pow(targetRatio, 1.35);
        }
        else
        {
            penalty = 0.40 + std::min(0.60, std::pow(targetRatio - 1.0, 0.90) * 0.60);
        }

        penalty += std::min(0.28, referenceRatio * 0.12);

        score.score = std::clamp(10.0 * (1.0 - penalty), 0.0, 10.0);
        score.signalClarity = std::clamp(1.0 - penalty * 0.78, 0.18, 1.0);
        score.confidence = score.signalClarity;
        score.proBandLow = -features.targetWindow;
        score.proBandHigh = features.targetWindow;

        result.categoryScores.push_back(score);
        scoreSum += score.score;
    }

    if (!result.categoryScores.empty())
    {
        result.overallRating = scoreSum / static_cast<double>(result.categoryScores.size());
    }

    return result;
}

} // namespace dawai::aifr3d_core
