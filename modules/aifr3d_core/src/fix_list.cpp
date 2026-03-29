#include "dawai/aifr3d_core/fix_list.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace dawai::aifr3d_core
{

namespace
{
std::string formatSigned(double value)
{
    std::ostringstream stream;
    stream << std::showpos << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string formatUnsigned(double value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

} // namespace

std::vector<FixListItem> FixListGenerator::generate(const FeatureSet& features,
                                                    const ScoringResult& scoring) const
{
    std::vector<FixListItem> out;

    std::vector<std::pair<CategoryScore, CategoryDeviation>> scored;
    scored.reserve(scoring.categoryScores.size());

    for (const auto& cat : scoring.categoryScores)
    {
        const auto it =
            std::find_if(features.categoryDeviations.begin(), features.categoryDeviations.end(),
                         [&](const CategoryDeviation& d) { return d.category == cat.category; });
        const CategoryDeviation deviation =
            (it == features.categoryDeviations.end()) ? CategoryDeviation{cat.category} : *it;
        scored.emplace_back(cat, deviation);
    }

    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b) { return a.first.score < b.first.score; });

    int priority = 1;
    for (const auto& entry : scored)
    {
        if (priority > 5)
        {
            break;
        }

        if (entry.second.deviation <= entry.first.proBandHigh)
        {
            continue;
        }

        FixListItem item;
        item.priority = priority++;
        item.category = entry.first.category;
        item.signalClarity = entry.first.signalClarity;
        item.confidence = entry.first.confidence;
        item.score = entry.first.score;
        item.targetSeverity = entry.second.deviation;
        item.referenceSeverity = entry.second.referenceDeviation;
        item.targetDelta = entry.second.targetDelta;
        item.referenceDelta = entry.second.referenceDelta;
        item.whatChanged = "target_delta=" + formatSigned(item.targetDelta) +
                           "; reference_delta=" + formatSigned(item.referenceDelta);
        item.whereYouStand = "target_severity=" + formatUnsigned(item.targetSeverity) +
                             "; reference_severity=" + formatUnsigned(item.referenceSeverity) +
                             "; target_window=+/-" + formatUnsigned(entry.first.proBandHigh) +
                             "; score=" + formatUnsigned(item.score);
        item.whatToDoNext.clear();
        item.options.clear();

        out.push_back(std::move(item));
    }

    if (out.empty())
    {
        FixListItem item;
        item.priority = 1;
        item.category = "Overall";
        item.signalClarity = 1.0;
        item.confidence = 1.0;
        item.score = scoring.overallRating;
        item.whatChanged = "all_scored_categories_within_target_window";
        item.whereYouStand = "target_window=+/-" + formatUnsigned(features.targetWindow) +
                             "; nearest_reference_variance_retained_in_score";
        item.whatToDoNext.clear();
        item.options.clear();
        out.push_back(std::move(item));
    }

    return out;
}

} // namespace dawai::aifr3d_core
