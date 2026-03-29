#include "ReferenceModel.h"

#include "../Common/AnalysisTypes.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace audiosynth::dsp
{

namespace
{
std::string toLower(std::string text);

FrequencyBalanceTarget buildFrequencyBalanceFromSignature(const ReferenceSignature& signature)
{
    const auto dbToEnergy = [](float db)
    {
        if (!std::isfinite(db) || db <= -140.0f)
        {
            return 0.0f;
        }
        return std::pow(10.0f, db / 10.0f);
    };

    const float sub = dbToEnergy(signature.subBandDb);
    const float bass = dbToEnergy(signature.lowBandDb);
    const float lowMid = dbToEnergy(signature.lowMidBandDb);
    const float mid = dbToEnergy(signature.midBandDb);
    const float highMid = dbToEnergy(signature.highMidBandDb);
    const float presence = dbToEnergy(signature.presenceBandDb);
    const float air = dbToEnergy(signature.airBandDb);
    const float total = sub + bass + lowMid + mid + highMid + presence + air;

    FrequencyBalanceTarget target;
    if (total <= 0.0f)
    {
        return target;
    }

    target.sub = sub / total;
    target.bass = bass / total;
    target.lowMid = lowMid / total;
    target.mid = mid / total;
    target.highMid = highMid / total;
    target.presence = presence / total;
    target.air = air / total;
    return target;
}

float bandValue(const FrequencyBalanceTarget& target, FrequencyBandId band)
{
    switch (band)
    {
    case FrequencyBandId::Sub:
        return target.sub;
    case FrequencyBandId::Bass:
        return target.bass;
    case FrequencyBandId::LowMid:
        return target.lowMid;
    case FrequencyBandId::Mid:
        return target.mid;
    case FrequencyBandId::HighMid:
        return target.highMid;
    case FrequencyBandId::Presence:
        return target.presence;
    case FrequencyBandId::Air:
        return target.air;
    }
    return -90.0f;
}

std::optional<FrequencyBandId> parseFrequencyBand(const std::string& raw)
{
    const auto band = toLower(raw);
    if (band == "sub")
        return FrequencyBandId::Sub;
    if (band == "bass")
        return FrequencyBandId::Bass;
    if (band == "low_mid" || band == "low-mid")
        return FrequencyBandId::LowMid;
    if (band == "mid")
        return FrequencyBandId::Mid;
    if (band == "high_mid" || band == "high-mid")
        return FrequencyBandId::HighMid;
    if (band == "presence")
        return FrequencyBandId::Presence;
    if (band == "air")
        return FrequencyBandId::Air;
    return std::nullopt;
}

uint8_t parseFrequencyStatusCode(const std::string& raw)
{
    const auto status = toLower(raw);
    if (status == "sub_excessive")
        return kFrequencyBalanceStatusSubExcessive;
    if (status == "sub_lacking")
        return kFrequencyBalanceStatusSubLacking;
    if (status == "bass_heavy")
        return kFrequencyBalanceStatusBassHeavy;
    if (status == "bass_thin")
        return kFrequencyBalanceStatusBassThin;
    if (status == "muddy")
        return kFrequencyBalanceStatusMuddy;
    if (status == "low_mid_hollow")
        return kFrequencyBalanceStatusLowMidHollow;
    if (status == "mid_heavy")
        return kFrequencyBalanceStatusMidHeavy;
    if (status == "mid_scooped")
        return kFrequencyBalanceStatusMidScooped;
    if (status == "harsh")
        return kFrequencyBalanceStatusHarsh;
    if (status == "dull")
        return kFrequencyBalanceStatusDull;
    if (status == "sibilant")
        return kFrequencyBalanceStatusSibilant;
    if (status == "dark")
        return kFrequencyBalanceStatusDark;
    return kFrequencyBalanceStatusNone;
}

bool parseSimpleThreshold(const std::string& condition, bool& greaterThan, float& threshold)
{
    const auto text = toLower(condition);
    const auto gt = text.find('>');
    const auto lt = text.find('<');
    const auto pos = gt != std::string::npos ? gt : lt;
    if (pos == std::string::npos)
    {
        return false;
    }

    greaterThan = gt != std::string::npos;
    try
    {
        threshold = std::stof(text.substr(pos + 1));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string normalizeMetricGenre(std::string genre)
{
    genre = toLower(std::move(genre));
    if (genre == "hip hop" || genre == "hiphop")
        return "hip-hop";
    if (genre == "trap")
        return "dubstep";
    return genre;
}

std::size_t profileSlotForGenreName(const std::string& genre)
{
    if (genre == "pop")
        return 0;
    if (genre == "edm")
        return 1;
    if (genre == "hip-hop")
        return 2;
    if (genre == "rap")
        return 3;
    if (genre == "dubstep")
        return 4;
    if (genre == "rock")
        return 5;
    return 0;
}

std::size_t profileSlotForId(GenreId id)
{
    switch (id)
    {
    case GenreId::Pop:
        return 0;
    case GenreId::Edm:
        return 1;
    case GenreId::HipHop:
        return 2;
    case GenreId::Rap:
        return 3;
    case GenreId::Dubstep:
        return 4;
    case GenreId::Rock:
        return 5;
    case GenreId::Unknown:
    default:
        return 0;
    }
}

constexpr float kFeatureDataEpsilon = 1.0e-4f;

bool hasUsableUnsignedFeature(float value)
{
    return std::isfinite(value) && value > kFeatureDataEpsilon && value <= 1.0f;
}

bool hasUsableSignedFeature(float value)
{
    return std::isfinite(value) && std::abs(value) > kFeatureDataEpsilon && value >= -1.0f &&
           value <= 1.0f;
}

bool hasUsableSpectralMetric(float value)
{
    return std::isfinite(value) && value > -80.0f;
}

void accumulateAverage(float value, float& sum, int& count)
{
    if (!std::isfinite(value))
    {
        return;
    }

    sum += value;
    ++count;
}

struct Accumulator
{
    FeatureFrame sum;
    int rmsCount = 0;
    int peakCount = 0;
    int crestCount = 0;
    int midEnergyCount = 0;
    int sideEnergyCount = 0;
    int correlationCount = 0;
    int transientCount = 0;
    int harshnessCount = 0;
    int mudCount = 0;
    int airCount = 0;
    float truePeakSum = 0.0f;
    int truePeakCount = 0;
    std::vector<float> spectrumBandSum;
    std::vector<int> spectrumBandCount;
    float subBandSum = 0.0f;
    int subBandCount = 0;
    float lowBandSum = 0.0f;
    int lowBandCount = 0;
    float lowMidBandSum = 0.0f;
    int lowMidBandCount = 0;
    float midBandSum = 0.0f;
    int midBandCount = 0;
    float highMidBandSum = 0.0f;
    int highMidBandCount = 0;
    float highBandSum = 0.0f;
    int highBandCount = 0;
    float presenceBandSum = 0.0f;
    int presenceBandCount = 0;
    float airBandSum = 0.0f;
    int airBandCount = 0;
    float spectralTiltSum = 0.0f;
    int spectralTiltCount = 0;
};

void accumulate(Accumulator& acc, const FeatureFrame& feature)
{
    accumulateAverage(feature.rms, acc.sum.rms, acc.rmsCount);
    accumulateAverage(feature.peak, acc.sum.peak, acc.peakCount);
    accumulateAverage(feature.crestDb, acc.sum.crestDb, acc.crestCount);
    accumulateAverage(feature.midEnergy, acc.sum.midEnergy, acc.midEnergyCount);
    accumulateAverage(feature.sideEnergy, acc.sum.sideEnergy, acc.sideEnergyCount);
    accumulateAverage(feature.correlation, acc.sum.correlation, acc.correlationCount);
    accumulateAverage(feature.transientDensity, acc.sum.transientDensity, acc.transientCount);
    accumulateAverage(feature.harshness, acc.sum.harshness, acc.harshnessCount);
    accumulateAverage(feature.mud, acc.sum.mud, acc.mudCount);
    accumulateAverage(feature.air, acc.sum.air, acc.airCount);
}

FeatureFrame average(const Accumulator& acc)
{
    FeatureFrame out{
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()};
    out.rms = (acc.rmsCount > 0) ? (acc.sum.rms / static_cast<float>(acc.rmsCount)) : out.rms;
    out.peak = (acc.peakCount > 0) ? (acc.sum.peak / static_cast<float>(acc.peakCount)) : out.peak;
    out.crestDb =
        (acc.crestCount > 0) ? (acc.sum.crestDb / static_cast<float>(acc.crestCount)) : out.crestDb;
    out.midEnergy = (acc.midEnergyCount > 0)
                        ? (acc.sum.midEnergy / static_cast<float>(acc.midEnergyCount))
                        : out.midEnergy;
    out.sideEnergy = (acc.sideEnergyCount > 0)
                         ? (acc.sum.sideEnergy / static_cast<float>(acc.sideEnergyCount))
                         : out.sideEnergy;
    out.correlation = (acc.correlationCount > 0)
                          ? (acc.sum.correlation / static_cast<float>(acc.correlationCount))
                          : out.correlation;
    out.transientDensity = (acc.transientCount > 0)
                               ? (acc.sum.transientDensity / static_cast<float>(acc.transientCount))
                               : out.transientDensity;
    out.harshness = (acc.harshnessCount > 0)
                        ? (acc.sum.harshness / static_cast<float>(acc.harshnessCount))
                        : out.harshness;
    out.mud = (acc.mudCount > 0) ? (acc.sum.mud / static_cast<float>(acc.mudCount)) : out.mud;
    out.air = (acc.airCount > 0) ? (acc.sum.air / static_cast<float>(acc.airCount)) : out.air;
    return out;
}

float averageTruePeak(const Accumulator& acc)
{
    if (acc.truePeakCount <= 0)
    {
        return -1.0f;
    }
    return acc.truePeakSum / static_cast<float>(acc.truePeakCount);
}

float averageBand(float sum, int count)
{
    if (count <= 0)
    {
        return -90.0f;
    }
    return sum / static_cast<float>(count);
}

float normalizedDelta(float current, float target, float scale)
{
    if (!std::isfinite(current) || !std::isfinite(target) || scale <= 0.0f)
    {
        return 1.0f;
    }

    return std::min(std::abs(current - target) / scale, 2.0f);
}

float dbToLinear(float db)
{
    if (!std::isfinite(db) || db <= -140.0f)
    {
        return 0.0f;
    }
    return std::pow(10.0f, db / 20.0f);
}

void accumulateSpectrum(Accumulator& acc, const ReferenceSignature& signature)
{
    if (signature.spectrumBandsDb.empty())
    {
    }
    else
    {
        if (acc.spectrumBandSum.size() < signature.spectrumBandsDb.size())
        {
            acc.spectrumBandSum.resize(signature.spectrumBandsDb.size(), 0.0f);
            acc.spectrumBandCount.resize(signature.spectrumBandsDb.size(), 0);
        }

        for (std::size_t index = 0; index < signature.spectrumBandsDb.size(); ++index)
        {
            if (!hasUsableSpectralMetric(signature.spectrumBandsDb[index]))
            {
                continue;
            }
            acc.spectrumBandSum[index] += signature.spectrumBandsDb[index];
            acc.spectrumBandCount[index] += 1;
        }
    }

    if (hasUsableSpectralMetric(signature.subBandDb))
    {
        acc.subBandSum += signature.subBandDb;
        ++acc.subBandCount;
    }
    if (hasUsableSpectralMetric(signature.lowBandDb))
    {
        acc.lowBandSum += signature.lowBandDb;
        ++acc.lowBandCount;
    }
    if (hasUsableSpectralMetric(signature.lowMidBandDb))
    {
        acc.lowMidBandSum += signature.lowMidBandDb;
        ++acc.lowMidBandCount;
    }
    if (hasUsableSpectralMetric(signature.midBandDb))
    {
        acc.midBandSum += signature.midBandDb;
        ++acc.midBandCount;
    }
    if (hasUsableSpectralMetric(signature.highMidBandDb))
    {
        acc.highMidBandSum += signature.highMidBandDb;
        ++acc.highMidBandCount;
    }
    if (hasUsableSpectralMetric(signature.highBandDb))
    {
        acc.highBandSum += signature.highBandDb;
        ++acc.highBandCount;
    }
    if (hasUsableSpectralMetric(signature.presenceBandDb))
    {
        acc.presenceBandSum += signature.presenceBandDb;
        ++acc.presenceBandCount;
    }
    if (hasUsableSpectralMetric(signature.airBandDb))
    {
        acc.airBandSum += signature.airBandDb;
        ++acc.airBandCount;
    }
    if (hasUsableSpectralMetric(signature.spectralTilt))
    {
        acc.spectralTiltSum += signature.spectralTilt;
        ++acc.spectralTiltCount;
    }
}

void deriveSpectralSignatureFromBands(ReferenceSignature& signature)
{
    if (signature.spectrumBandsDb.empty())
    {
        signature.subBandDb = -90.0f;
        signature.lowBandDb = -90.0f;
        signature.lowMidBandDb = -90.0f;
        signature.midBandDb = -90.0f;
        signature.highMidBandDb = -90.0f;
        signature.highBandDb = -90.0f;
        signature.presenceBandDb = -90.0f;
        signature.airBandDb = -90.0f;
        signature.spectralTilt = -90.0f;
        return;
    }

    const auto averageRange = [&](float startRatio, float endRatio)
    {
        const auto total = signature.spectrumBandsDb.size();
        const std::size_t begin = std::min<std::size_t>(
            total - 1, static_cast<std::size_t>(std::floor(startRatio * static_cast<float>(total))));
        const std::size_t end = std::min<std::size_t>(
            total, static_cast<std::size_t>(std::ceil(endRatio * static_cast<float>(total))));
        if (begin >= end)
        {
            return signature.spectrumBandsDb[begin];
        }

        float sum = 0.0f;
        for (std::size_t i = begin; i < end; ++i)
        {
            sum += signature.spectrumBandsDb[i];
        }
        return sum / static_cast<float>(end - begin);
    };

    signature.subBandDb = averageRange(20.0f / 24000.0f, 60.0f / 24000.0f);
    signature.lowBandDb = averageRange(60.0f / 24000.0f, 200.0f / 24000.0f);
    signature.lowMidBandDb = averageRange(200.0f / 24000.0f, 800.0f / 24000.0f);
    signature.midBandDb = averageRange(800.0f / 24000.0f, 2000.0f / 24000.0f);
    signature.highMidBandDb = averageRange(2000.0f / 24000.0f, 6000.0f / 24000.0f);
    signature.highBandDb = averageRange(6000.0f / 24000.0f, 16000.0f / 24000.0f);
    signature.presenceBandDb = signature.highMidBandDb;
    signature.airBandDb = signature.highBandDb;
    signature.spectralTilt =
        ((signature.highMidBandDb + signature.highBandDb) * 0.5f) -
        ((signature.subBandDb + signature.lowBandDb + signature.lowMidBandDb) / 3.0f);
}

std::string toLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c)
                   { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool containsAny(const std::string& haystack, const std::vector<std::string>& needles)
{
    for (const auto& needle : needles)
    {
        if (haystack.find(needle) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

float parseTruePeakDbTp(const nlohmann::json& profile)
{
    if (profile.contains("truePeakDbTP"))
    {
        return static_cast<float>(profile.value("truePeakDbTP", -1.0));
    }
    if (profile.contains("true_peak_dbTP"))
    {
        return static_cast<float>(profile.value("true_peak_dbTP", -1.0));
    }
    if (profile.contains("true_peak_dbtp"))
    {
        return static_cast<float>(profile.value("true_peak_dbtp", -1.0));
    }
    return -1.0f;
}

float parseFloat(const nlohmann::json& profile, const char* key, float fallback)
{
    if (!profile.contains(key))
    {
        return fallback;
    }
    try
    {
        return static_cast<float>(profile.at(key).get<double>());
    }
    catch (...)
    {
        return fallback;
    }
}

std::optional<GenreId> parseGenreToken(const std::string& raw)
{
    const auto g = toLower(raw);
    if (g.empty())
    {
        return std::nullopt;
    }

    if (containsAny(g, {"hip-hop", "hiphop", "hip hop"}))
    {
        return GenreId::HipHop;
    }
    if (containsAny(g, {"dubstep", "brostep", "riddim", "wobble", "bass music"}))
    {
        return GenreId::Dubstep;
    }
    if (containsAny(g, {"rock", "metal", "punk", "grunge", "hardcore"}))
    {
        return GenreId::Rock;
    }
    if (containsAny(g, {"rap"}))
    {
        return GenreId::Rap;
    }
    if (containsAny(g,
                    {"edm", "electro", "house", "techno", "trance", "dnb", "drum&bass",
                     "drum and bass"}))
    {
        return GenreId::Edm;
    }
    if (containsAny(g, {"trap", "808"}))
    {
        // Pool requirement uses dubstep as the bass-heavy slot.
        return GenreId::Dubstep;
    }
    if (containsAny(g, {"pop"}))
    {
        return GenreId::Pop;
    }
    return std::nullopt;
}

GenreId inferGenreFromMetadata(const nlohmann::json& profile)
{
    if (profile.contains("genre") && profile["genre"].is_string())
    {
        if (const auto parsed = parseGenreToken(profile["genre"].get<std::string>());
            parsed.has_value())
        {
            return *parsed;
        }
    }

    const auto text = toLower(profile.value("sourcePath", "") + " " + profile.value("title", ""));
    if (const auto parsed = parseGenreToken(text); parsed.has_value())
    {
        return *parsed;
    }

    return GenreId::Pop;
}

std::optional<std::size_t> genreSlot(GenreId id)
{
    switch (id)
    {
    case GenreId::Pop:
        return 0;
    case GenreId::Edm:
        return 1;
    case GenreId::HipHop:
        return 2;
    case GenreId::Rap:
        return 3;
    case GenreId::Dubstep:
        return 4;
    case GenreId::Rock:
        return 5;
    case GenreId::Unknown:
        break;
    }
    return std::nullopt;
}

bool pathStartsWith(const std::filesystem::path& path, const std::filesystem::path& prefix)
{
    auto pathIt = path.begin();
    auto prefixIt = prefix.begin();
    for (; prefixIt != prefix.end(); ++prefixIt, ++pathIt)
    {
        if (pathIt == path.end() || *pathIt != *prefixIt)
        {
            return false;
        }
    }
    return true;
}

bool manifestSourceAllowed(const std::string& raw)
{
    const auto normalized = toLower(raw);
    if (normalized.empty())
    {
        return false;
    }
    return normalized.rfind("assets/reference_intake/", 0) == 0;
}

std::filesystem::path canonicalProfilePath(const std::filesystem::path& base,
                                           const std::string& relativePath)
{
    std::error_code ec;
    auto resolved = std::filesystem::weakly_canonical(base / relativePath, ec);
    if (ec)
    {
        resolved = std::filesystem::path(base / relativePath).lexically_normal();
    }
    return resolved;
}

bool profilePathAllowed(const std::filesystem::path& manifestPath,
                        const std::filesystem::path& profilePath)
{
    std::error_code ec;
    auto allowedRoot = std::filesystem::weakly_canonical(manifestPath.parent_path() / "profiles", ec);
    if (ec)
    {
        allowedRoot = (manifestPath.parent_path() / "profiles").lexically_normal();
    }
    return pathStartsWith(profilePath.lexically_normal(), allowedRoot.lexically_normal());
}

} // namespace

ReferenceModel::ReferenceModel()
{
    const auto makeSignature = [](FeatureFrame feature, float truePeakDbTP)
    {
        ReferenceSignature signature;
        signature.feature = feature;
        signature.truePeakDbTP = truePeakDbTP;
        return signature;
    };

    m_profiles[0].profile.id = GenreId::Pop;
    m_profiles[0].profile.name = "Pop";
    m_profiles[0].profile.mean = makeSignature(
        FeatureFrame{0.21f, 0.80f, 7.6f, 0.70f, 0.30f, 0.36f, 0.46f, 0.34f, 0.31f, 0.56f},
        -1.05f);

    m_profiles[1].profile.id = GenreId::Edm;
    m_profiles[1].profile.name = "EDM";
    m_profiles[1].profile.mean = makeSignature(
        FeatureFrame{0.24f, 0.90f, 9.1f, 0.64f, 0.36f, 0.22f, 0.58f, 0.41f, 0.26f, 0.62f},
        -1.25f);

    m_profiles[2].profile.id = GenreId::HipHop;
    m_profiles[2].profile.name = "Hip-Hop";
    m_profiles[2].profile.mean = makeSignature(
        FeatureFrame{0.23f, 0.88f, 8.5f, 0.74f, 0.26f, 0.20f, 0.50f, 0.30f, 0.36f, 0.50f},
        -1.34f);

    m_profiles[3].profile.id = GenreId::Rap;
    m_profiles[3].profile.name = "Rap";
    m_profiles[3].profile.mean = makeSignature(
        FeatureFrame{0.23f, 0.88f, 8.4f, 0.76f, 0.24f, 0.18f, 0.52f, 0.32f, 0.35f, 0.47f},
        -1.38f);

    m_profiles[4].profile.id = GenreId::Dubstep;
    m_profiles[4].profile.name = "Dubstep";
    m_profiles[4].profile.mean = makeSignature(
        FeatureFrame{0.24f, 0.92f, 9.0f, 0.66f, 0.34f, 0.16f, 0.58f, 0.39f, 0.34f, 0.58f},
        -1.48f);

    m_profiles[5].profile.id = GenreId::Rock;
    m_profiles[5].profile.name = "Rock";
    m_profiles[5].profile.mean = makeSignature(
        FeatureFrame{0.22f, 0.86f, 10.1f, 0.81f, 0.19f, 0.28f, 0.54f, 0.36f, 0.31f, 0.49f},
        -1.12f);

    for (auto& slot : m_profiles)
    {
        slot.profile.match = slot.profile.mean;
        slot.profile.mean.frequencyBalance = buildFrequencyBalanceFromSignature(slot.profile.mean);
        slot.profile.match.frequencyBalance = buildFrequencyBalanceFromSignature(slot.profile.match);
    }

    m_unknown = GenreProfile{
        GenreId::Unknown,
        "Unknown",
        makeSignature(
            FeatureFrame{0.22f, 0.85f, 8.0f, 0.67f, 0.33f, 0.25f, 0.50f, 0.35f, 0.32f, 0.54f},
            -1.20f),
        makeSignature(
            FeatureFrame{0.22f, 0.85f, 8.0f, 0.67f, 0.33f, 0.25f, 0.50f, 0.35f, 0.32f, 0.54f},
            -1.20f),
    };
    m_unknown.mean.frequencyBalance = buildFrequencyBalanceFromSignature(m_unknown.mean);
    m_unknown.match.frequencyBalance = buildFrequencyBalanceFromSignature(m_unknown.match);

    m_active = m_profiles[0].profile;
    m_referencePoolReady = false;
    m_referencePoolBlocked = false;
}

bool ReferenceModel::hasUsableSpectralValue(float value)
{
    return std::isfinite(value) && value > -80.0f;
}

bool ReferenceModel::hasUsableFrequencyEnergy(float value)
{
    return std::isfinite(value) && value >= 0.0f && value <= 1.0f;
}

bool ReferenceModel::signatureHasUsableFeatureData(const ReferenceSignature& signature)
{
    const auto& feature = signature.feature;
    return (std::isfinite(feature.crestDb) && feature.crestDb > 0.0f) ||
           hasUsableUnsignedFeature(feature.transientDensity) ||
           hasUsableUnsignedFeature(feature.sideEnergy) ||
           hasUsableUnsignedFeature(feature.midEnergy) ||
           hasUsableSignedFeature(feature.correlation) ||
           hasUsableUnsignedFeature(feature.harshness) ||
           hasUsableUnsignedFeature(feature.mud) || hasUsableUnsignedFeature(feature.air);
}

bool ReferenceModel::signatureHasUsableSpectralData(const ReferenceSignature& signature)
{
    return hasUsableSpectralValue(signature.subBandDb) || hasUsableSpectralValue(signature.lowBandDb) ||
           hasUsableSpectralValue(signature.lowMidBandDb) ||
           hasUsableSpectralValue(signature.midBandDb) ||
           hasUsableSpectralValue(signature.highMidBandDb) ||
           hasUsableSpectralValue(signature.highBandDb) ||
           hasUsableSpectralValue(signature.presenceBandDb) ||
           hasUsableSpectralValue(signature.airBandDb) ||
           std::any_of(signature.spectrumBandsDb.begin(), signature.spectrumBandsDb.end(),
                       [](float value) { return ReferenceModel::hasUsableSpectralValue(value); });
}

bool ReferenceModel::signatureUsable(const ReferenceSignature& signature)
{
    return signatureHasUsableFeatureData(signature) || signatureHasUsableSpectralData(signature);
}

bool ReferenceModel::loadCanonicalPool(const std::filesystem::path& manifestPath)
{
    m_manifestPath = manifestPath;
    m_referencePoolReady = false;
    m_referencePoolBlocked = false;

    if (!std::filesystem::exists(manifestPath))
    {
        return false;
    }

    for (auto& slot : m_profiles)
    {
        slot.references.clear();
        slot.profile.match = slot.profile.mean;
        slot.profile.mean.frequencyBalance = buildFrequencyBalanceFromSignature(slot.profile.mean);
        slot.profile.match.frequencyBalance = slot.profile.mean.frequencyBalance;
    }

    std::ifstream input(manifestPath);
    if (!input)
    {
        return false;
    }

    nlohmann::json manifest;
    input >> manifest;
    if (!manifest.is_object())
    {
        return false;
    }

    std::array<Accumulator, 6> accumulators{};
    const auto references =
        manifest.contains("references") && manifest["references"].is_array()
            ? manifest["references"]
            : nlohmann::json::array();

    auto parseProfileSignature = [&](const nlohmann::json& profile) -> std::optional<ReferenceSignature>
    {
        const nlohmann::json* profileObject = &profile;
        if (profile.is_array() && !profile.empty() && profile.front().is_object())
        {
            profileObject = &profile.front();
        }

        if (!profileObject->is_object())
        {
            return std::nullopt;
        }

        const float missing = std::numeric_limits<float>::quiet_NaN();
        ReferenceSignature signature;
        FeatureFrame frame{missing, missing, missing, missing, missing,
                           missing, missing, missing, missing, missing};

        if (profileObject->contains("rmsDb") || profileObject->contains("rms_db"))
        {
            frame.rms =
                dbToLinear(parseFloat(*profileObject, "rmsDb",
                                      parseFloat(*profileObject, "rms_db", -70.0f)));
        }
        if (profileObject->contains("peakDbfs") || profileObject->contains("peak_dbfs"))
        {
            frame.peak =
                dbToLinear(parseFloat(*profileObject, "peakDbfs",
                                      parseFloat(*profileObject, "peak_dbfs", -120.0f)));
        }

        frame.crestDb = parseFloat(*profileObject, "crestFactorDb",
                                   parseFloat(*profileObject, "crest_factor_db", missing));
        frame.transientDensity = parseFloat(*profileObject, "transientDensity",
                                            parseFloat(*profileObject, "transient_density", missing));
        frame.correlation = parseFloat(*profileObject, "correlation", missing);

        const float storedMidEnergy =
            parseFloat(*profileObject, "midEnergy", parseFloat(*profileObject, "mid_energy", missing));
        const float storedSideEnergy =
            parseFloat(*profileObject, "sideEnergy",
                       parseFloat(*profileObject, "side_energy", missing));
        if (std::isfinite(storedMidEnergy) || std::isfinite(storedSideEnergy))
        {
            const float mid = juce::jmax(0.0f, std::isfinite(storedMidEnergy) ? storedMidEnergy : 0.0f);
            const float side = juce::jmax(0.0f, std::isfinite(storedSideEnergy) ? storedSideEnergy : 0.0f);
            const float total = mid + side;
            if (total > 1.0e-6f)
            {
                frame.midEnergy = clamp01(mid / total);
                frame.sideEnergy = clamp01(side / total);
            }
        }
        else
        {
            const float width = parseFloat(*profileObject, "stereoWidth",
                                           parseFloat(*profileObject, "stereo_width", missing));
            if (std::isfinite(width))
            {
                frame.sideEnergy = clamp01(width / (1.0f + width));
                frame.midEnergy = clamp01(1.0f - frame.sideEnergy);
            }
        }

        const auto crestDist =
            profileObject->value("crestFactorDistribution", std::vector<double>{});
        if (!crestDist.empty())
        {
            frame.crestDb = static_cast<float>(crestDist.front());
        }

        const auto spectrum = profileObject->value("spectrumBandsDb", std::vector<double>{});
        if (!spectrum.empty())
        {
            signature.spectrumBandsDb.reserve(spectrum.size());
            for (double value : spectrum)
            {
                signature.spectrumBandsDb.push_back(static_cast<float>(value));
            }
            deriveSpectralSignatureFromBands(signature);
        }

        signature.subBandDb = parseFloat(*profileObject, "subBandDb", signature.subBandDb);
        signature.lowBandDb = parseFloat(*profileObject, "lowBandDb", signature.lowBandDb);
        signature.lowMidBandDb =
            parseFloat(*profileObject, "lowMidBandDb", signature.lowMidBandDb);
        signature.midBandDb = parseFloat(*profileObject, "midBandDb", signature.midBandDb);
        signature.highMidBandDb =
            parseFloat(*profileObject, "highMidBandDb", signature.highMidBandDb);
        signature.highBandDb = parseFloat(*profileObject, "highBandDb", signature.highBandDb);
        signature.presenceBandDb =
            parseFloat(*profileObject, "presenceBandDb", signature.presenceBandDb);
        signature.airBandDb = parseFloat(*profileObject, "airBandDb", signature.airBandDb);
        signature.spectralTilt = parseFloat(*profileObject, "spectralTiltDb",
                                            parseFloat(*profileObject, "spectralTilt",
                                                       signature.spectralTilt));

        if (!hasUsableSpectralValue(signature.highMidBandDb) &&
            hasUsableSpectralValue(signature.presenceBandDb))
        {
            signature.highMidBandDb = signature.presenceBandDb;
        }
        if (!hasUsableSpectralValue(signature.highBandDb) &&
            hasUsableSpectralValue(signature.airBandDb))
        {
            signature.highBandDb = signature.airBandDb;
        }

        frame.mud = hasUsableSpectralValue(signature.lowMidBandDb) &&
                            hasUsableSpectralValue(signature.midBandDb)
                        ? clamp01((signature.lowMidBandDb - signature.midBandDb + 6.0f) / 18.0f)
                        : missing;
        frame.harshness = hasUsableSpectralValue(signature.highMidBandDb) &&
                                  hasUsableSpectralValue(signature.midBandDb)
                              ? clamp01((signature.highMidBandDb - signature.midBandDb + 6.0f) /
                                        18.0f)
                              : missing;
        frame.air = hasUsableSpectralValue(signature.highBandDb) &&
                            hasUsableSpectralValue(signature.highMidBandDb)
                        ? clamp01((signature.highBandDb - signature.highMidBandDb + 6.0f) / 18.0f)
                        : missing;

        signature.feature = frame;
        signature.truePeakDbTP = parseTruePeakDbTp(*profileObject);
        signature.frequencyBalance = buildFrequencyBalanceFromSignature(signature);
        return signatureUsable(signature) ? std::optional<ReferenceSignature>(signature)
                                          : std::nullopt;
    };

    for (const auto& item : references)
    {
        if (!item.is_object())
        {
            continue;
        }

        const auto sourcePath = item.value("source_path", item.value("sourcePath", ""));
        if (!manifestSourceAllowed(sourcePath))
        {
            m_referencePoolBlocked = true;
            continue;
        }

        const auto profileRelative = item.value("profile_path", item.value("profilePath", ""));
        if (profileRelative.empty())
        {
            continue;
        }

        const auto profilePath = canonicalProfilePath(manifestPath.parent_path(), profileRelative);
        if (!profilePathAllowed(manifestPath, profilePath) || !std::filesystem::exists(profilePath))
        {
            m_referencePoolBlocked = true;
            continue;
        }

        std::ifstream profileInput(profilePath);
        if (!profileInput)
        {
            continue;
        }

        nlohmann::json profilePayload;
        profileInput >> profilePayload;
        auto signature = parseProfileSignature(profilePayload);
        if (!signature.has_value())
        {
            continue;
        }

        GenreId id = GenreId::Unknown;
        if (item.contains("genre") && item["genre"].is_string())
        {
            if (const auto parsed = parseGenreToken(item["genre"].get<std::string>()); parsed.has_value())
            {
                id = *parsed;
            }
        }
        if (id == GenreId::Unknown)
        {
            id = inferGenreFromMetadata(profilePayload);
        }

        const auto slot = genreSlot(id);
        if (!slot.has_value())
        {
            continue;
        }

        accumulate(accumulators[*slot], signature->feature);
        accumulateSpectrum(accumulators[*slot], *signature);
        if (signature->truePeakDbTP > -80.0f && signature->truePeakDbTP <= 3.0f)
        {
            accumulators[*slot].truePeakSum += signature->truePeakDbTP;
            ++accumulators[*slot].truePeakCount;
        }
        m_profiles[*slot].references.push_back(*signature);
    }

    bool changed = false;
    for (std::size_t i = 0; i < m_profiles.size(); ++i)
    {
        if (!m_profiles[i].references.empty())
        {
            m_profiles[i].profile.mean.feature = average(accumulators[i]);
            const float meanTruePeak = averageTruePeak(accumulators[i]);
            if (meanTruePeak > -80.0f)
            {
                m_profiles[i].profile.mean.truePeakDbTP = meanTruePeak;
            }
            m_profiles[i].profile.mean.spectrumBandsDb.clear();
            m_profiles[i].profile.mean.spectrumBandsDb.reserve(accumulators[i].spectrumBandSum.size());
            for (std::size_t band = 0; band < accumulators[i].spectrumBandSum.size(); ++band)
            {
                m_profiles[i].profile.mean.spectrumBandsDb.push_back(
                    averageBand(accumulators[i].spectrumBandSum[band],
                                accumulators[i].spectrumBandCount[band]));
            }
            m_profiles[i].profile.mean.subBandDb =
                averageBand(accumulators[i].subBandSum, accumulators[i].subBandCount);
            m_profiles[i].profile.mean.lowBandDb =
                averageBand(accumulators[i].lowBandSum, accumulators[i].lowBandCount);
            m_profiles[i].profile.mean.lowMidBandDb =
                averageBand(accumulators[i].lowMidBandSum, accumulators[i].lowMidBandCount);
            m_profiles[i].profile.mean.midBandDb =
                averageBand(accumulators[i].midBandSum, accumulators[i].midBandCount);
            m_profiles[i].profile.mean.highMidBandDb =
                averageBand(accumulators[i].highMidBandSum, accumulators[i].highMidBandCount);
            m_profiles[i].profile.mean.highBandDb =
                averageBand(accumulators[i].highBandSum, accumulators[i].highBandCount);
            m_profiles[i].profile.mean.presenceBandDb =
                averageBand(accumulators[i].presenceBandSum, accumulators[i].presenceBandCount);
            m_profiles[i].profile.mean.airBandDb =
                averageBand(accumulators[i].airBandSum, accumulators[i].airBandCount);
            m_profiles[i].profile.mean.spectralTilt =
                averageBand(accumulators[i].spectralTiltSum, accumulators[i].spectralTiltCount);
            if (!m_profiles[i].references.empty())
            {
                m_profiles[i].profile.match = m_profiles[i].references.front();
            }
            else
            {
                m_profiles[i].profile.match = m_profiles[i].profile.mean;
            }
            m_profiles[i].profile.mean.frequencyBalance =
                buildFrequencyBalanceFromSignature(m_profiles[i].profile.mean);
            m_profiles[i].profile.match.frequencyBalance =
                buildFrequencyBalanceFromSignature(m_profiles[i].profile.match);
            if (!signatureUsable(m_profiles[i].profile.match))
            {
                m_profiles[i].profile.match = m_profiles[i].profile.mean;
                m_profiles[i].profile.match.frequencyBalance =
                    buildFrequencyBalanceFromSignature(m_profiles[i].profile.match);
            }
            changed = true;
        }
    }

    m_referencePoolReady = changed;
    return changed && !m_referencePoolBlocked;
}

bool ReferenceModel::loadMetricConfig(const std::filesystem::path& configPath)
{
    if (!std::filesystem::exists(configPath))
    {
        return false;
    }

    std::ifstream input(configPath);
    if (!input)
    {
        return false;
    }

    nlohmann::json payload;
    input >> payload;
    const auto metrics = payload.contains("metrics") && payload["metrics"].is_object() ? payload["metrics"]
                                                                                        : nlohmann::json::object();
    const auto frequencyMetric = metrics.contains("frequency_balance") ? metrics["frequency_balance"]
                                                                       : payload.value("frequency_balance", nlohmann::json::object());

    m_frequencyBalanceRules.clear();
    for (const auto& ruleJson : frequencyMetric.value("classification_rules", nlohmann::json::array()))
    {
        if (!ruleJson.is_object())
        {
            continue;
        }
        const auto band = parseFrequencyBand(ruleJson.value("band", ""));
        if (!band.has_value())
        {
            continue;
        }
        FrequencyBalanceRule rule;
        rule.band = *band;
        if (!parseSimpleThreshold(ruleJson.value("condition", ""), rule.greaterThan, rule.threshold))
        {
            continue;
        }
        rule.statusCode = parseFrequencyStatusCode(ruleJson.value("status", ""));
        rule.severity = static_cast<uint8_t>(std::clamp(ruleJson.value("severity", 0), 0, 255));
        rule.advisoryKey = ruleJson.value("advisory_key", "");
        m_frequencyBalanceRules.push_back(std::move(rule));
    }

    const auto loudnessMetric = metrics.contains("integrated_loudness") ? metrics["integrated_loudness"]
                                                                        : payload.value("integrated_loudness", nlohmann::json::object());
    const auto loudnessTargets = loudnessMetric.value("reference_targets", nlohmann::json::object());
    if (loudnessTargets.contains("streaming") && loudnessTargets["streaming"].is_object())
    {
        m_integratedLoudnessReferenceLufs =
            static_cast<float>(loudnessTargets["streaming"].value("spotify", -14.0));
    }

    const auto dynamicMetric = metrics.contains("dynamic_range") ? metrics["dynamic_range"]
                                                                 : payload.value("dynamic_range", nlohmann::json::object());
    const auto dynamicTargets = dynamicMetric.value("reference_targets", nlohmann::json::object());
    if (dynamicTargets.contains("genres") && dynamicTargets["genres"].is_object())
    {
        const auto genres = dynamicTargets["genres"];
        for (auto it = genres.begin(); it != genres.end(); ++it)
        {
            const auto genre = normalizeMetricGenre(it.key());
            m_dynamicRangeTargets[profileSlotForGenreName(genre)] =
                static_cast<float>(it.value().get<double>());
        }
    }

    return !m_frequencyBalanceRules.empty() ||
           std::abs(m_integratedLoudnessReferenceLufs + 14.0f) > 0.001f;
}

FrequencyBalanceClassification
ReferenceModel::classifyFrequencyBalance(const FrequencyBalanceTarget& balance) const
{
    for (const auto& rule : m_frequencyBalanceRules)
    {
        const float value = bandValue(balance, rule.band);
        if (!hasUsableFrequencyEnergy(value))
        {
            continue;
        }
        const bool matches = rule.greaterThan ? (value > rule.threshold) : (value < rule.threshold);
        if (matches)
        {
            return {rule.statusCode, rule.severity, rule.advisoryKey};
        }
    }
    return {};
}

LoudnessClassification ReferenceModel::classifyIntegratedLoudness(float integratedLufs) const
{
    const float reference = m_integratedLoudnessReferenceLufs;
    if (integratedLufs > reference + 4.0f)
        return {kIntegratedLoudnessStatusCriticallyLoud, 3, reference, "loudness_critically_loud"};
    if (integratedLufs > reference + 1.5f)
        return {kIntegratedLoudnessStatusTooLoud, 2, reference, "loudness_too_loud"};
    if (integratedLufs > reference)
        return {kIntegratedLoudnessStatusSlightlyLoud, 1, reference, "loudness_slightly_loud"};
    if (integratedLufs >= reference - 1.5f)
        return {kIntegratedLoudnessStatusOptimal, 0, reference, "loudness_optimal"};
    if (integratedLufs >= reference - 4.0f)
        return {kIntegratedLoudnessStatusTooQuiet, 2, reference, "loudness_too_quiet"};
    return {kIntegratedLoudnessStatusCriticallyQuiet, 3, reference, "loudness_critically_quiet"};
}

DynamicRangeClassification ReferenceModel::classifyDynamicRange(float dynamicRangeDb) const
{
    const float reference = m_dynamicRangeTargets[profileSlotForId(m_active.id)];
    if (dynamicRangeDb >= 14.0f)
        return {kDynamicRangeStatusHighlyDynamic, 0, reference, "dr_highly_dynamic"};
    if (dynamicRangeDb >= 10.0f)
        return {kDynamicRangeStatusDynamic, 0, reference, "dr_dynamic"};
    if (dynamicRangeDb >= 7.0f)
        return {kDynamicRangeStatusBalanced, 0, reference, "dr_balanced"};
    if (dynamicRangeDb >= 5.0f)
        return {kDynamicRangeStatusCompressed, 1, reference, "dr_compressed"};
    if (dynamicRangeDb >= 3.0f)
        return {kDynamicRangeStatusHeavilyCompressed, 2, reference, "dr_heavily_compressed"};
    return {kDynamicRangeStatusBrickWalled, 3, reference, "dr_brick_walled"};
}

float ReferenceModel::distanceToProfile(const FeatureFrame& feature, const SpectralFrame& spectral,
                                        const ReferenceSignature& signature)
{
    if (!signatureUsable(signature))
    {
        return std::numeric_limits<float>::max();
    }

    const auto& mean = signature.feature;
    float weightedDistance = 0.0f;
    float totalWeight = 0.0f;

    const auto addFeatureDistance = [&](float current, float target, float scale, float weight)
    {
        if (!std::isfinite(current) || !std::isfinite(target))
        {
            return;
        }

        weightedDistance += weight * normalizedDelta(current, target, scale);
        totalWeight += weight;
    };

    addFeatureDistance(feature.crestDb, mean.crestDb, 10.0f, 0.16f);
    addFeatureDistance(feature.transientDensity, mean.transientDensity, 0.6f, 0.10f);
    addFeatureDistance(feature.midEnergy, mean.midEnergy, 0.5f, 0.08f);
    addFeatureDistance(feature.sideEnergy, mean.sideEnergy, 0.5f, 0.08f);
    addFeatureDistance(feature.correlation, mean.correlation, 1.0f, 0.06f);
    addFeatureDistance(feature.harshness, mean.harshness, 1.0f, 0.06f);
    addFeatureDistance(feature.mud, mean.mud, 1.0f, 0.06f);
    addFeatureDistance(feature.air, mean.air, 1.0f, 0.05f);

    const auto addBandDistance = [&](float current, float target, float weight)
    {
        if (!hasUsableSpectralValue(current) || !hasUsableSpectralValue(target))
        {
            return;
        }

        weightedDistance += weight * normalizedDelta(current, target, 9.0f);
        totalWeight += weight;
    };

    addBandDistance(spectral.subBandDb, signature.subBandDb, 0.12f);
    addBandDistance(spectral.lowBandDb, signature.lowBandDb, 0.11f);
    addBandDistance(spectral.lowMidBandDb, signature.lowMidBandDb, 0.12f);
    addBandDistance(spectral.midBandDb, signature.midBandDb, 0.10f);
    addBandDistance(spectral.highMidBandDb, signature.highMidBandDb, 0.12f);
    addBandDistance(spectral.highBandDb, signature.highBandDb, 0.10f);

    if (std::isfinite(spectral.spectralTiltDb) && !signature.spectrumBandsDb.empty())
    {
        weightedDistance += 0.08f * normalizedDelta(spectral.spectralTiltDb, signature.spectralTilt, 9.0f);
        totalWeight += 0.08f;
    }

    if (!spectral.averagedBinsDb.empty() && !signature.spectrumBandsDb.empty())
    {
        const std::size_t count =
            std::min<std::size_t>(spectral.averagedBinsDb.size(), signature.spectrumBandsDb.size());
        float vectorDistance = 0.0f;
        float vectorWeight = 0.0f;
        for (std::size_t i = 0; i < count; ++i)
        {
            float bandWeight = 1.0f;
            if (i < count / 4)
            {
                bandWeight = 1.30f;
            }
            else if (i >= (count * 3) / 4)
            {
                bandWeight = 1.15f;
            }

            vectorDistance +=
                bandWeight * normalizedDelta(spectral.averagedBinsDb[i], signature.spectrumBandsDb[i], 12.0f);
            vectorWeight += bandWeight;
        }

        if (vectorWeight > 0.0f)
        {
            weightedDistance += 0.34f * (vectorDistance / vectorWeight);
            totalWeight += 0.34f;
        }
    }

    return (totalWeight > 0.0f) ? (weightedDistance / totalWeight)
                                : std::numeric_limits<float>::max();
}

GenreProfile ReferenceModel::matchGenre(GenreId id, const FeatureFrame& feature,
                                        const SpectralFrame& spectral, float& outConfidence) const
{
    outConfidence = 0.0f;
    const auto slot = genreSlot(id);
    if (!slot.has_value())
    {
        return m_unknown;
    }

    const auto& profileSet = m_profiles[*slot];
    if (!signatureUsable(profileSet.profile.mean))
    {
        return m_unknown;
    }

    float bestDistance = distanceToProfile(feature, spectral, profileSet.profile.mean);
    ReferenceSignature bestMatch = profileSet.profile.mean;
    for (const auto& reference : profileSet.references)
    {
        const float distance = distanceToProfile(feature, spectral, reference);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestMatch = reference;
        }
    }

    auto result = profileSet.profile;
    result.match = signatureUsable(bestMatch) ? bestMatch : profileSet.profile.mean;
    outConfidence = clamp01(1.0f - std::min(bestDistance, 1.0f));
    return result;
}

GenreProfile ReferenceModel::detectGenre(const FeatureFrame& feature, const SpectralFrame& spectral,
                                         float& outConfidence) const
{
    if (!m_referencePoolReady)
    {
        outConfidence = 0.0f;
        return m_unknown;
    }

    float bestDistance = std::numeric_limits<float>::max();
    GenreProfile best = m_unknown;

    for (const auto& slot : m_profiles)
    {
        if (!signatureUsable(slot.profile.mean))
        {
            continue;
        }

        float genreBestDistance = std::numeric_limits<float>::max();
        ReferenceSignature genreBestMatch = slot.profile.mean;

        if (slot.references.empty())
        {
            genreBestDistance = distanceToProfile(feature, spectral, slot.profile.mean);
        }
        else
        {
            for (const auto& reference : slot.references)
            {
                const float distance = distanceToProfile(feature, spectral, reference);
                if (distance < genreBestDistance)
                {
                    genreBestDistance = distance;
                    genreBestMatch = reference;
                }
            }
        }

        if (genreBestDistance < bestDistance)
        {
            bestDistance = genreBestDistance;
            best = slot.profile;
            best.match = genreBestMatch;
        }
    }

    outConfidence = clamp01(1.0f - bestDistance);
    return best;
}

bool ReferenceModel::activeProfileUsable() const noexcept
{
    return m_referencePoolReady && signatureUsable(m_active.mean) && signatureUsable(m_active.match);
}

const char* ReferenceModel::genreName(GenreId id) noexcept
{
    switch (id)
    {
    case GenreId::Unknown:
        return "Unknown";
    case GenreId::Pop:
        return "Pop";
    case GenreId::Edm:
        return "EDM";
    case GenreId::HipHop:
        return "Hip-Hop";
    case GenreId::Rap:
        return "Rap";
    case GenreId::Dubstep:
        return "Dubstep";
    case GenreId::Rock:
        return "Rock";
    }

    return "Unknown";
}

} // namespace audiosynth::dsp
