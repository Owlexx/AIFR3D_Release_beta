#pragma once

#include "dawai/aifr3d_core/fix_list.hpp"

#include <filesystem>
#include <string>

namespace dawai::aifr3d_core
{

class AnalysisWriter
{
  public:
    [[nodiscard]] std::string buildResultJson(const FeatureSet& features,
                                              const ScoringResult& scoring) const;
    [[nodiscard]] std::string buildFixListJson(const FeatureSet& features,
                                               const ScoringResult& scoring,
                                               const std::vector<FixListItem>& fixList) const;
    bool write(const std::filesystem::path& analysisDir, const FeatureSet& features,
               const ScoringResult& scoring, const std::vector<FixListItem>& fixList) const;
};

} // namespace dawai::aifr3d_core
