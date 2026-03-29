#include "dawai/advisory_layer/adviser.hpp"

#include <algorithm>
#include <filesystem>

namespace dawai::advisory_layer
{

bool AnalysisSandbox::canWrite(const std::filesystem::path& path) const
{
    if (m_allowExternalWrites)
    {
        return true;
    }

    const auto base = std::filesystem::absolute(m_baseDir).lexically_normal();
    const auto parent =
        path.parent_path().empty() ? std::filesystem::path(".") : path.parent_path();
    const auto target = std::filesystem::absolute(parent).lexically_normal();
    const auto mismatch = std::mismatch(base.begin(), base.end(), target.begin(), target.end());
    return mismatch.first == base.end();
}

} // namespace dawai::advisory_layer
