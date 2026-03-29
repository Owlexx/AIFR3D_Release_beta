#include "dawai/reference_engine/genre_detector.hpp"

#include "dawai/reference_engine/profile_cache.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <numeric>
#include <limits>
#include <unordered_map>
#include <vector>

namespace dawai::reference_engine
{

namespace
{
std::string normalizeGenre(std::string genre)
{
    std::transform(genre.begin(), genre.end(), genre.begin(), [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });

    if (genre == "hiphop" || genre == "hip hop")
    {
        return "hip-hop";
    }
    if (genre == "trap")
    {
        return "dubstep";
    }
    return genre;
}

double detectDistance(const dawai::metering::MeterSnapshot& snapshot,
                      const ReferenceProfile& reference)
{
    const auto bins =
        std::min(snapshot.spectrum.averagedBinsDb.size(), reference.spectrumBandsDb.size());

    double spectrumPenalty = 0.0;
    if (bins > 0)
    {
        for (std::size_t i = 0; i < bins; ++i)
        {
            spectrumPenalty += std::abs(snapshot.spectrum.averagedBinsDb[i] -
                                        reference.spectrumBandsDb[i]);
        }
        spectrumPenalty /= static_cast<double>(bins);
        spectrumPenalty = std::clamp(spectrumPenalty / 12.0, 0.0, 1.0);
    }

    const double lufsPenalty =
        std::clamp(std::abs(snapshot.loudness.integratedLufs - reference.integratedLufs) / 12.0,
                   0.0, 1.0);
    const double truePeakPenalty =
        (reference.truePeakDbtp > -80.0)
            ? std::clamp(std::abs(snapshot.loudness.truePeakDbtp - reference.truePeakDbtp) / 6.0,
                         0.0, 1.0)
            : 0.0;

    const double refCrest = reference.crestFactorDistribution.empty()
                                ? 10.0
                                : reference.crestFactorDistribution.front();
    const double crestPenalty =
        std::clamp(std::abs(snapshot.dynamics.crestFactorDb - refCrest) / 10.0, 0.0, 1.0);

    const double widthPenalty =
        std::clamp(std::abs(snapshot.stereo.width - reference.stereoWidth), 0.0, 1.0);

    return 0.35 * spectrumPenalty + 0.20 * lufsPenalty + 0.15 * truePeakPenalty +
           0.18 * crestPenalty + 0.12 * widthPenalty;
}

ReferenceProfile buildGenreMean(const std::string& genre,
                                const std::vector<ReferenceProfile>& profiles)
{
    ReferenceProfile mean;
    mean.id = "genre-mean-" + genre;
    mean.title = genre + " mean";
    mean.sourcePath = "derived::genre-mean";

    if (profiles.empty())
    {
        return mean;
    }

    double lufsSum = 0.0;
    double stereoWidthSum = 0.0;
    double transientDensitySum = 0.0;
    double truePeakSum = 0.0;
    std::size_t truePeakCount = 0;
    double crestSum = 0.0;
    std::size_t crestCount = 0;
    std::size_t maxBins = 0;

    for (const auto& profile : profiles)
    {
        lufsSum += profile.integratedLufs;
        stereoWidthSum += profile.stereoWidth;
        transientDensitySum += profile.transientDensity;
        maxBins = std::max(maxBins, profile.spectrumBandsDb.size());

        if (profile.truePeakDbtp > -80.0)
        {
            truePeakSum += profile.truePeakDbtp;
            ++truePeakCount;
        }

        if (!profile.crestFactorDistribution.empty())
        {
            crestSum += profile.crestFactorDistribution.front();
            ++crestCount;
        }
    }

    mean.integratedLufs = lufsSum / static_cast<double>(profiles.size());
    mean.stereoWidth = stereoWidthSum / static_cast<double>(profiles.size());
    mean.transientDensity = transientDensitySum / static_cast<double>(profiles.size());
    if (truePeakCount > 0)
    {
        mean.truePeakDbtp = truePeakSum / static_cast<double>(truePeakCount);
    }
    if (crestCount > 0)
    {
        mean.crestFactorDistribution = {crestSum / static_cast<double>(crestCount)};
    }

    mean.spectrumBandsDb.assign(maxBins, 0.0);
    std::vector<std::size_t> bandCounts(maxBins, 0);
    for (const auto& profile : profiles)
    {
        for (std::size_t index = 0; index < profile.spectrumBandsDb.size(); ++index)
        {
            mean.spectrumBandsDb[index] += profile.spectrumBandsDb[index];
            ++bandCounts[index];
        }
    }
    for (std::size_t index = 0; index < mean.spectrumBandsDb.size(); ++index)
    {
        if (bandCounts[index] > 0)
        {
            mean.spectrumBandsDb[index] /= static_cast<double>(bandCounts[index]);
        }
    }

    return mean;
}

} // namespace

bool GenreDetector::load(const std::filesystem::path& manifestPath,
                         const std::filesystem::path& profileRootDirectory)
{
    std::ifstream input(manifestPath);
    if (!input)
    {
        return false;
    }

    nlohmann::json manifest;
    input >> manifest;
    if (!manifest.is_object() || !manifest.contains("references"))
    {
        return false;
    }

    std::unordered_map<std::string, std::vector<ReferenceProfile>> grouped;
    ProfileCache cache;
    m_referenceCount = 0;

    for (const auto& ref : manifest.value("references", nlohmann::json::array()))
    {
        if (!ref.is_object())
        {
            continue;
        }

        const auto genre = normalizeGenre(ref.value("genre", "unknown"));
        auto profilePath = std::filesystem::path(ref.value("profile_path", ""));
        if (profilePath.empty())
        {
            continue;
        }

        if (profilePath.is_relative())
        {
            profilePath = profileRootDirectory / profilePath;
        }

        const auto loaded = cache.load(profilePath);
        if (loaded.empty())
        {
            continue;
        }

        grouped[genre].push_back(loaded.front());
        ++m_referenceCount;
    }

    std::unordered_map<std::string, ReferenceProfile> means;
    for (const auto& [genre, profiles] : grouped)
    {
        means.emplace(genre, buildGenreMean(genre, profiles));
    }

    m_genreProfiles = std::move(grouped);
    m_genreMeans = std::move(means);
    return !m_genreProfiles.empty();
}

std::optional<GenreDetection>
GenreDetector::detect(const dawai::metering::MeterSnapshot& snapshot) const
{
    if (m_genreProfiles.empty())
    {
        return std::nullopt;
    }

    double bestDistance = std::numeric_limits<double>::max();
    const std::string* bestGenre = nullptr;
    const ReferenceProfile* bestProfile = nullptr;

    for (const auto& [genre, profiles] : m_genreProfiles)
    {
        for (const auto& profile : profiles)
        {
            const double distance = detectDistance(snapshot, profile);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestGenre = &genre;
                bestProfile = &profile;
            }
        }
    }

    if (bestGenre == nullptr || bestProfile == nullptr)
    {
        return std::nullopt;
    }

    GenreDetection detection;
    detection.genre = *bestGenre;
    detection.confidence = std::clamp(1.0 - bestDistance, 0.0, 1.0);
    detection.referenceMatch = *bestProfile;
    if (const auto meanIt = m_genreMeans.find(*bestGenre); meanIt != m_genreMeans.end())
    {
        detection.referenceMean = meanIt->second;
    }
    else
    {
        detection.referenceMean = *bestProfile;
    }
    return detection;
}

} // namespace dawai::reference_engine
