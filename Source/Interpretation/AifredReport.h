#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct AifredIssue
{
    std::string id;
    std::string category;
    std::string title;
    std::string explanation;
    std::string suggestedAction;
    std::string impact;
    float confidence = 0.0f;
};

struct AifredReport
{
    std::string summaryStatus;
    std::string haloLabel;
    std::string haloDetail;
    std::string tonalSummary;
    std::string trendStatus;
    std::string analysisState;
    float overallAlignment = 0.0f;
    float confidence = 0.0f;
    std::uint32_t flags = 0;
    std::vector<AifredIssue> issues;
};
