#pragma once

#include "dawai/metering/metering_engine.hpp"
#include "dawai/reference_engine/reference_profile.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace dawai::reference_engine
{

struct GenreDetection
{
    std::string genre = "Unknown";
    double confidence = 0.0;
    ReferenceProfile referenceMatch;
    ReferenceProfile referenceMean;
};

class GenreDetector
{
  public:
    bool load(const std::filesystem::path& manifestPath,
              const std::filesystem::path& profileRootDirectory);

    [[nodiscard]] bool empty() const noexcept
    {
        return m_genreProfiles.empty();
    }

    [[nodiscard]] std::size_t referenceCount() const noexcept
    {
        return m_referenceCount;
    }

    [[nodiscard]] std::optional<GenreDetection>
    detect(const dawai::metering::MeterSnapshot& snapshot) const;

  private:
    std::size_t m_referenceCount = 0;
    std::unordered_map<std::string, std::vector<ReferenceProfile>> m_genreProfiles;
    std::unordered_map<std::string, ReferenceProfile> m_genreMeans;
};

} // namespace dawai::reference_engine
