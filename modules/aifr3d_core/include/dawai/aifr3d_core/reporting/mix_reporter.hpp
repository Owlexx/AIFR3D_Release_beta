#pragma once

#include "dawai/aifr3d_core/fix_list.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dawai::aifr3d_core::reporting
{

struct MixReport
{
    std::string timestampUtc;
    std::string sessionName;
    std::string referenceUsed;
    double rating = 0.0;
    std::vector<FixListItem> fixList;
};

class MixReporter
{
  public:
    bool writeJsonAndMarkdown(const std::filesystem::path& folder, const MixReport& report) const;
    [[nodiscard]] std::optional<MixReport> readJson(const std::filesystem::path& jsonPath) const;

    [[nodiscard]] std::vector<MixReport> loadLastN(const std::filesystem::path& folder,
                                                   std::size_t count) const;
};

struct ReportDelta
{
    double ratingDelta = 0.0;
    int fixCountDelta = 0;
};

[[nodiscard]] ReportDelta compareReports(const MixReport& current, const MixReport& previous);

} // namespace dawai::aifr3d_core::reporting
