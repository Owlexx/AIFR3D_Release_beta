#pragma once

#include "../Common/AnalysisTypes.h"

#include <filesystem>
#include <vector>

namespace aifred::dsp
{

class JsonReportWriter
{
  public:
    bool write(const std::filesystem::path& path, const std::vector<AnalysisFrame>& frames) const;
};

} // namespace aifred::dsp
