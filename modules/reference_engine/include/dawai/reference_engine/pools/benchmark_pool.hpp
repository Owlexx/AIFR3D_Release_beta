#pragma once

#include "dawai/reference_engine/reference_profile.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dawai::reference_engine::pools
{

struct PoolMetadata
{
    std::string name;
    std::string genre;
    std::optional<int> bpmMin;
    std::optional<int> bpmMax;
    std::optional<std::string> musicalKey;
};

struct PoolEntry
{
    std::string profileId;
    std::filesystem::path profilePath;
};

struct MetricBand
{
    double mean = 0.0;
    double p25 = 0.0;
    double p75 = 0.0;
};

struct PoolAggregate
{
    MetricBand integratedLufs;
    MetricBand crestFactor;
    MetricBand stereoWidth;
    std::vector<MetricBand> spectrumBands;
};

class BenchmarkPool
{
  public:
    bool load(const std::filesystem::path& poolJsonPath);
    [[nodiscard]] const PoolMetadata& metadata() const noexcept
    {
        return m_metadata;
    }
    [[nodiscard]] const std::vector<PoolEntry>& entries() const noexcept
    {
        return m_entries;
    }

  private:
    PoolMetadata m_metadata;
    std::vector<PoolEntry> m_entries;
};

PoolAggregate computePoolAggregate(const std::vector<ReferenceProfile>& profiles);

bool buildReferenceCache(const std::filesystem::path& poolJsonPath,
                         const std::filesystem::path& outputPath);

} // namespace dawai::reference_engine::pools
