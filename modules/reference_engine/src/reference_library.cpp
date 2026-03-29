#include "dawai/reference_engine/reference_library.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace dawai::reference_engine
{

namespace
{
ReferenceProfile parseProfile(const nlohmann::json& j)
{
    const auto parseFrequencyBalance = [](const nlohmann::json& value)
    {
        FrequencyBalanceProfile balance;
        if (!value.is_object())
        {
            return balance;
        }
        balance.subEnergy = value.value("sub_energy", 0.0);
        balance.bassEnergy = value.value("bass_energy", 0.0);
        balance.lowMidEnergy = value.value("low_mid_energy", 0.0);
        balance.midEnergy = value.value("mid_energy", 0.0);
        balance.highMidEnergy = value.value("high_mid_energy", 0.0);
        balance.presenceEnergy = value.value("presence_energy", 0.0);
        balance.airEnergy = value.value("air_energy", 0.0);
        return balance;
    };

    ReferenceProfile p;
    p.id = j.value("id", "");
    p.title = j.value("title", "");
    p.sourcePath = j.value("sourcePath", "");
    p.spectrumBandsDb = j.value("spectrumBandsDb", std::vector<double>{});
    p.spectralTiltDb = j.value("spectralTiltDb", j.value("spectralTilt", 0.0));
    p.subBandDb = j.value("subBandDb", -90.0);
    p.lowBandDb = j.value("lowBandDb", -90.0);
    p.lowMidBandDb = j.value("lowMidBandDb", -90.0);
    p.midBandDb = j.value("midBandDb", -90.0);
    p.highMidBandDb = j.value("highMidBandDb", j.value("presenceBandDb", -90.0));
    p.presenceBandDb = j.value("presenceBandDb", -90.0);
    p.airBandDb = j.value("airBandDb", -90.0);
    p.integratedLufs = j.value("integratedLufs", -23.0);
    p.shortTermLufs = j.value("shortTermLufs", j.value("short_term_lufs", -23.0));
    p.truePeakDbtp =
        j.value("truePeakDbtp", j.value("true_peak_dbtp", j.value("truePeakDbTP", -1.0)));
    p.shortTermLufsDistribution = j.value("shortTermLufsDistribution", std::vector<double>{});
    p.rmsDb = j.value("rmsDb", j.value("rms_db", -70.0));
    p.peakDbfs = j.value("peakDbfs", j.value("peak_dbfs", -120.0));
    p.crestFactorDb = j.value("crestFactorDb", j.value("crest_factor_db", 0.0));
    p.crestFactorDistribution = j.value("crestFactorDistribution", std::vector<double>{});
    p.correlation = j.value("correlation", 1.0);
    p.stereoWidth = j.value("stereoWidth", 0.0);
    p.midEnergy = j.value("midEnergy", 0.0);
    p.sideEnergy = j.value("sideEnergy", 0.0);
    p.phaseRisk = j.value("phaseRisk", 0.0);
    p.subMonoIntegrity = j.value("subMonoIntegrity", 1.0);
    p.transientDensity = j.value("transientDensity", 0.0);
    p.frequencyBalance = parseFrequencyBalance(j.value("frequency_balance", nlohmann::json::object()));
    return p;
}

} // namespace

void ReferenceLibrary::addProfile(ReferenceProfile profile)
{
    const auto it =
        std::find_if(m_profiles.begin(), m_profiles.end(),
                     [&](const ReferenceProfile& existing) { return existing.id == profile.id; });
    if (it != m_profiles.end())
    {
        *it = std::move(profile);
    }
    else
    {
        m_profiles.push_back(std::move(profile));
    }
}

bool ReferenceLibrary::loadFolder(const std::filesystem::path& directory)
{
    m_profiles.clear();
    if (!std::filesystem::exists(directory))
    {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory))
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

        nlohmann::json json;
        input >> json;
        if (json.is_array())
        {
            for (const auto& item : json)
            {
                if (!item.is_object())
                {
                    continue;
                }
                addProfile(parseProfile(item));
            }
            continue;
        }

        if (json.is_object())
        {
            addProfile(parseProfile(json));
        }
    }

    return !m_profiles.empty();
}

std::vector<ReferenceProfile> ReferenceLibrary::allProfiles() const
{
    return m_profiles;
}

std::optional<ReferenceProfile> ReferenceLibrary::findById(const std::string& id) const
{
    const auto it = std::find_if(m_profiles.begin(), m_profiles.end(),
                                 [&](const ReferenceProfile& profile) { return profile.id == id; });
    if (it == m_profiles.end())
    {
        return std::nullopt;
    }
    return *it;
}

} // namespace dawai::reference_engine
