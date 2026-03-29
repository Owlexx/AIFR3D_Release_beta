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

namespace aifred::dsp
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
    return 0.0f;
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

struct Accumulator
{
    FeatureFrame sum;
    int count = 0;
    int spectralCount = 0;
    float truePeakSum = 0.0f;
    int truePeakCount = 0;
    std::vector<float> spectrumBandSum;
    std::vector<int> spectrumBandCount;
    float subBandSum = 0.0f;
    float lowBandSum = 0.0f;
    float lowMidBandSum = 0.0f;
    float midBandSum = 0.0f;
    float highMidBandSum = 0.0f;
    float highBandSum = 0.0f;
    float presenceBandSum = 0.0f;
    float airBandSum = 0.0f;
    float spectralTiltSum = 0.0f;
};

void accumulate(Accumulator& acc, const FeatureFrame& feature)
{
    acc.sum.rms += feature.rms;
    acc.sum.peak += feature.peak;
    acc.sum.crestDb += feature.crestDb;
    acc.sum.midEnergy += feature.midEnergy;
    acc.sum.sideEnergy += feature.sideEnergy;
    acc.sum.correlation += feature.correlation;
    acc.sum.transientDensity += feature.transientDensity;
    acc.sum.harshness += feature.harshness;
    acc.sum.mud += feature.mud;
    acc.sum.air += feature.air;
    ++acc.count;
}

FeatureFrame average(const Accumulator& acc)
{
    FeatureFrame out;
    if (acc.count <= 0)
    {
        return out;
    }

    const float inv = 1.0f / static_cast<float>(acc.count);
    out.rms = acc.sum.rms * inv;
    out.peak = acc.sum.peak * inv;
    out.crestDb = acc.sum.crestDb * inv;
    out.midEnergy = acc.sum.midEnergy * inv;
    out.sideEnergy = acc.sum.sideEnergy * inv;
    out.correlation = acc.sum.correlation * inv;
    out.transientDensity = acc.sum.transientDensity * inv;
    out.harshness = acc.sum.harshness * inv;
    out.mud = acc.sum.mud * inv;
    out.air = acc.sum.air * inv;
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

bool hasUsableSpectralValue(float value)
{
    return std::isfinite(value) && value > -80.0f;
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
        return;
    }

    ++acc.spectralCount;
    if (acc.spectrumBandSum.size() < signature.spectrumBandsDb.size())
    {
        acc.spectrumBandSum.resize(signature.spectrumBandsDb.size(), 0.0f);
        acc.spectrumBandCount.resize(signature.spectrumBandsDb.size(), 0);
    }

    for (std::size_t index = 0; index < signature.spectrumBandsDb.size(); ++index)
    {
        acc.spectrumBandSum[index] += signature.spectrumBandsDb[index];
        acc.spectrumBandCount[index] += 1;
    }

    acc.subBandSum += signature.subBandDb;
    acc.lowBandSum += signature.lowBandDb;
    acc.lowMidBandSum += signature.lowMidBandDb;
    acc.midBandSum += signature.midBandDb;
    acc.highMidBandSum += signature.highMidBandDb;
    acc.highBandSum += signature.highBandDb;
    acc.presenceBandSum += signature.presenceBandDb;
    acc.airBandSum += signature.airBandDb;
    acc.spectralTiltSum += signature.spectralTilt;
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
        signature.spectralTilt = 0.0f;
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
}

bool ReferenceModel::loadGenreMeansFromProfiles(const std::filesystem::path& profileDirectory)
{
    if (!std::filesystem::exists(profileDirectory))
    {
        return false;
    }

    std::array<Accumulator, 6> accumulators{};
    for (auto& slot : m_profiles)
    {
        slot.references.clear();
    }

    for (const auto& entry : std::filesystem::directory_iterator(profileDirectory))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
        {
            continue;
        }

        std::ifstream input(entry.path());
        if (!input)
        {
            continue;
        }

        nlohmann::json payload;
        input >> payload;

        auto processProfile = [&](const nlohmann::json& profile)
        {
            if (!profile.is_object())
            {
                return;
            }

            ReferenceSignature signature;
            FeatureFrame frame;
            frame.rms = dbToLinear(parseFloat(profile, "rmsDb", parseFloat(profile, "rms_db", -70.0f)));
            frame.peak = dbToLinear(parseFloat(profile, "peakDbfs", parseFloat(profile, "peak_dbfs", -120.0f)));
            frame.crestDb = parseFloat(profile, "crestFactorDb", parseFloat(profile, "crest_factor_db",
                                                                             static_cast<float>(profile.value("integratedLufs", -14.0) * -0.55)));
            frame.transientDensity = static_cast<float>(profile.value("transientDensity", 0.5));
            frame.correlation = parseFloat(profile, "correlation", 0.2f);

            const float storedMidEnergy = parseFloat(profile, "midEnergy", -1.0f);
            const float storedSideEnergy = parseFloat(profile, "sideEnergy", -1.0f);
            if (storedMidEnergy >= 0.0f || storedSideEnergy >= 0.0f)
            {
                const float mid = juce::jmax(0.0f, storedMidEnergy);
                const float side = juce::jmax(0.0f, storedSideEnergy);
                const float total = juce::jmax(1.0e-6f, mid + side);
                frame.midEnergy = clamp01(mid / total);
                frame.sideEnergy = clamp01(side / total);
            }
            else
            {
                const float width = static_cast<float>(profile.value("stereoWidth", 0.3));
                frame.sideEnergy = clamp01(width / (1.0f + width));
                frame.midEnergy = clamp01(1.0f - frame.sideEnergy);
            }

            const auto crestDist = profile.value("crestFactorDistribution", std::vector<double>{});
            if (!crestDist.empty())
            {
                frame.crestDb = static_cast<float>(crestDist.front());
            }

            const auto spectrum = profile.value("spectrumBandsDb", std::vector<double>{});
            if (!spectrum.empty())
            {
                signature.spectrumBandsDb.reserve(spectrum.size());
                for (double value : spectrum)
                {
                    signature.spectrumBandsDb.push_back(static_cast<float>(value));
                }
                deriveSpectralSignatureFromBands(signature);
            }

            signature.lowBandDb = parseFloat(profile, "lowBandDb", signature.lowBandDb);
            signature.lowMidBandDb = parseFloat(profile, "lowMidBandDb", signature.lowMidBandDb);
            signature.presenceBandDb = parseFloat(profile, "presenceBandDb", signature.presenceBandDb);
            signature.airBandDb = parseFloat(profile, "airBandDb", signature.airBandDb);
            signature.spectralTilt = parseFloat(profile, "spectralTiltDb",
                                                parseFloat(profile, "spectralTilt", signature.spectralTilt));

            if (!hasUsableSpectralValue(signature.highMidBandDb) &&
                hasUsableSpectralValue(signature.presenceBandDb))
            {
                signature.highMidBandDb = signature.presenceBandDb;
            }
            if (!hasUsableSpectralValue(signature.highBandDb) && hasUsableSpectralValue(signature.airBandDb))
            {
                signature.highBandDb = signature.airBandDb;
            }

            frame.mud = hasUsableSpectralValue(signature.lowMidBandDb) && hasUsableSpectralValue(signature.midBandDb)
                ? clamp01((signature.lowMidBandDb - signature.midBandDb + 6.0f) / 18.0f)
                : 0.35f;
            frame.harshness = hasUsableSpectralValue(signature.highMidBandDb) && hasUsableSpectralValue(signature.midBandDb)
                ? clamp01((signature.highMidBandDb - signature.midBandDb + 6.0f) / 18.0f)
                : 0.35f;
            frame.air = hasUsableSpectralValue(signature.highBandDb) && hasUsableSpectralValue(signature.highMidBandDb)
                ? clamp01((signature.highBandDb - signature.highMidBandDb + 6.0f) / 18.0f)
                : 0.50f;

            signature.feature = frame;
            signature.truePeakDbTP = parseTruePeakDbTp(profile);

            if (const auto slot = genreSlot(inferGenreFromMetadata(profile)); slot.has_value())
            {
                accumulate(accumulators[*slot], frame);
                accumulateSpectrum(accumulators[*slot], signature);
                if (signature.truePeakDbTP > -80.0f && signature.truePeakDbTP <= 3.0f)
                {
                    accumulators[*slot].truePeakSum += signature.truePeakDbTP;
                    ++accumulators[*slot].truePeakCount;
                }
                m_profiles[*slot].references.push_back(signature);
            }
        };

        if (payload.is_array())
        {
            for (const auto& item : payload)
            {
                processProfile(item);
            }
        }
        else
        {
            processProfile(payload);
        }
    }

    bool changed = false;
    for (std::size_t i = 0; i < m_profiles.size(); ++i)
    {
        if (accumulators[i].count > 0)
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
            const int spectralCount = accumulators[i].spectralCount;
            m_profiles[i].profile.mean.subBandDb =
                averageBand(accumulators[i].subBandSum, spectralCount);
            m_profiles[i].profile.mean.lowBandDb =
                averageBand(accumulators[i].lowBandSum, spectralCount);
            m_profiles[i].profile.mean.lowMidBandDb =
                averageBand(accumulators[i].lowMidBandSum, spectralCount);
            m_profiles[i].profile.mean.midBandDb =
                averageBand(accumulators[i].midBandSum, spectralCount);
            m_profiles[i].profile.mean.highMidBandDb =
                averageBand(accumulators[i].highMidBandSum, spectralCount);
            m_profiles[i].profile.mean.highBandDb =
                averageBand(accumulators[i].highBandSum, spectralCount);
            m_profiles[i].profile.mean.presenceBandDb =
                averageBand(accumulators[i].presenceBandSum, spectralCount);
            m_profiles[i].profile.mean.airBandDb =
                averageBand(accumulators[i].airBandSum, spectralCount);
            m_profiles[i].profile.mean.spectralTilt =
                averageBand(accumulators[i].spectralTiltSum, spectralCount);
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
            changed = true;
        }
    }

    return changed;
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
    const auto referenceTargets = frequencyMetric.value("reference_targets", nlohmann::json::object());
    for (auto it = referenceTargets.begin(); it != referenceTargets.end(); ++it)
    {
        const auto genre = normalizeMetricGenre(it.key());
        FrequencyBalanceTarget target;
        if (it.value().is_object())
        {
            target.sub = static_cast<float>(it.value().value("sub", 0.0));
            target.bass = static_cast<float>(it.value().value("bass", 0.0));
            target.lowMid = static_cast<float>(it.value().value("low_mid", 0.0));
            target.mid = static_cast<float>(it.value().value("mid", 0.0));
            target.highMid = static_cast<float>(it.value().value("high_mid", 0.0));
            target.presence = static_cast<float>(it.value().value("presence", 0.0));
            target.air = static_cast<float>(it.value().value("air", 0.0));
        }
        m_profiles[profileSlotForGenreName(genre)].profile.mean.frequencyBalance = target;
    }

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

    return !m_frequencyBalanceRules.empty() || m_integratedLoudnessReferenceLufs != -14.0f;
}

FrequencyBalanceClassification
ReferenceModel::classifyFrequencyBalance(const FrequencyBalanceTarget& balance) const
{
    for (const auto& rule : m_frequencyBalanceRules)
    {
        const float value = bandValue(balance, rule.band);
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
    const auto& mean = signature.feature;

    const float crest = std::abs(feature.crestDb - mean.crestDb) / 10.0f;
    const float transient = std::abs(feature.transientDensity - mean.transientDensity) / 0.6f;
    const float mid = std::abs(feature.midEnergy - mean.midEnergy) / 0.5f;
    const float side = std::abs(feature.sideEnergy - mean.sideEnergy) / 0.5f;
    const float corr = std::abs(feature.correlation - mean.correlation) / 1.0f;
    const float harsh = std::abs(feature.harshness - mean.harshness) / 1.0f;
    const float mud = std::abs(feature.mud - mean.mud) / 1.0f;
    const float air = std::abs(feature.air - mean.air) / 1.0f;

    float weightedDistance =
        0.16f * crest + 0.10f * transient + 0.08f * mid + 0.08f * side + 0.06f * corr +
        0.06f * harsh + 0.06f * mud + 0.05f * air;
    float totalWeight = 0.65f;

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

    return (totalWeight > 0.0f) ? (weightedDistance / totalWeight) : 1.0f;
}

GenreProfile ReferenceModel::detectGenre(const FeatureFrame& feature, const SpectralFrame& spectral,
                                         float& outConfidence) const
{
    float bestDistance = std::numeric_limits<float>::max();
    GenreProfile best = m_unknown;

    for (const auto& slot : m_profiles)
    {
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

} // namespace aifred::dsp
