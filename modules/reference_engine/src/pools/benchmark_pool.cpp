#include "dawai/reference_engine/pools/benchmark_pool.hpp"

#include "dawai/reference_engine/profile_cache.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace dawai::reference_engine::pools
{

namespace
{
double percentile(std::vector<double> values, double p)
{
    if (values.empty())
    {
        return 0.0;
    }

    std::sort(values.begin(), values.end());
    const double idx = p * static_cast<double>(values.size() - 1);
    const auto lower = static_cast<std::size_t>(idx);
    const auto upper = std::min<std::size_t>(values.size() - 1, lower + 1);
    const double frac = idx - static_cast<double>(lower);
    return values[lower] * (1.0 - frac) + values[upper] * frac;
}

MetricBand fromSamples(const std::vector<double>& samples)
{
    MetricBand band;
    if (samples.empty())
    {
        return band;
    }

    double sum = 0.0;
    for (double v : samples)
    {
        sum += v;
    }

    band.mean = sum / static_cast<double>(samples.size());
    band.p25 = percentile(samples, 0.25);
    band.p75 = percentile(samples, 0.75);
    return band;
}

} // namespace

bool BenchmarkPool::load(const std::filesystem::path& poolJsonPath)
{
    std::ifstream input(poolJsonPath);
    if (!input)
    {
        return false;
    }

    nlohmann::json j;
    input >> j;

    m_metadata.name = j.value("name", "Unnamed Pool");
    m_metadata.genre = j.value("genre", "Unknown");

    if (j.contains("bpmMin"))
    {
        m_metadata.bpmMin = j["bpmMin"].get<int>();
    }
    if (j.contains("bpmMax"))
    {
        m_metadata.bpmMax = j["bpmMax"].get<int>();
    }
    if (j.contains("key"))
    {
        m_metadata.musicalKey = j["key"].get<std::string>();
    }

    m_entries.clear();
    for (const auto& entry : j.value("entries", nlohmann::json::array()))
    {
        PoolEntry poolEntry;
        poolEntry.profileId = entry.value("profileId", "");
        poolEntry.profilePath = entry.value("profilePath", "");
        m_entries.push_back(std::move(poolEntry));
    }

    return !m_entries.empty();
}

PoolAggregate computePoolAggregate(const std::vector<ReferenceProfile>& profiles)
{
    PoolAggregate aggregate;
    if (profiles.empty())
    {
        return aggregate;
    }

    std::vector<double> lufs;
    std::vector<double> crest;
    std::vector<double> width;

    const std::size_t bandCount = profiles.front().spectrumBandsDb.size();
    std::vector<std::vector<double>> bandSamples(bandCount);

    for (const auto& profile : profiles)
    {
        lufs.push_back(profile.integratedLufs);
        width.push_back(profile.stereoWidth);
        if (!profile.crestFactorDistribution.empty())
        {
            crest.push_back(profile.crestFactorDistribution.front());
        }
        for (std::size_t i = 0; i < std::min(bandCount, profile.spectrumBandsDb.size()); ++i)
        {
            bandSamples[i].push_back(profile.spectrumBandsDb[i]);
        }
    }

    aggregate.integratedLufs = fromSamples(lufs);
    aggregate.crestFactor = fromSamples(crest);
    aggregate.stereoWidth = fromSamples(width);

    aggregate.spectrumBands.reserve(bandCount);
    for (const auto& samples : bandSamples)
    {
        aggregate.spectrumBands.push_back(fromSamples(samples));
    }

    return aggregate;
}

bool buildReferenceCache(const std::filesystem::path& poolJsonPath,
                         const std::filesystem::path& outputPath)
{
    BenchmarkPool pool;
    if (!pool.load(poolJsonPath))
    {
        return false;
    }

    ProfileCache cache;
    std::vector<ReferenceProfile> profiles;

    for (const auto& entry : pool.entries())
    {
        const auto loaded = cache.load(entry.profilePath);
        profiles.insert(profiles.end(), loaded.begin(), loaded.end());
    }

    if (profiles.empty())
    {
        return false;
    }

    return cache.save(outputPath, profiles);
}

} // namespace dawai::reference_engine::pools
