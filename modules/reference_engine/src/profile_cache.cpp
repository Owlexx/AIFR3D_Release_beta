#include "dawai/reference_engine/profile_cache.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace dawai::reference_engine
{

namespace
{
nlohmann::json toJson(const ReferenceProfile& p)
{
    return {
        {"id", p.id},
        {"title", p.title},
        {"sourcePath", p.sourcePath},
        {"spectrumBandsDb", p.spectrumBandsDb},
        {"spectralTiltDb", p.spectralTiltDb},
        {"subBandDb", p.subBandDb},
        {"lowBandDb", p.lowBandDb},
        {"lowMidBandDb", p.lowMidBandDb},
        {"midBandDb", p.midBandDb},
        {"highMidBandDb", p.highMidBandDb},
        {"presenceBandDb", p.presenceBandDb},
        {"airBandDb", p.airBandDb},
        {"integratedLufs", p.integratedLufs},
        {"shortTermLufs", p.shortTermLufs},
        {"truePeakDbtp", p.truePeakDbtp},
        {"shortTermLufsDistribution", p.shortTermLufsDistribution},
        {"rmsDb", p.rmsDb},
        {"peakDbfs", p.peakDbfs},
        {"crestFactorDb", p.crestFactorDb},
        {"crestFactorDistribution", p.crestFactorDistribution},
        {"correlation", p.correlation},
        {"stereoWidth", p.stereoWidth},
        {"midEnergy", p.midEnergy},
        {"sideEnergy", p.sideEnergy},
        {"phaseRisk", p.phaseRisk},
        {"subMonoIntegrity", p.subMonoIntegrity},
        {"transientDensity", p.transientDensity},
        {"frequency_balance",
         {
             {"sub_energy", p.frequencyBalance.subEnergy},
             {"bass_energy", p.frequencyBalance.bassEnergy},
             {"low_mid_energy", p.frequencyBalance.lowMidEnergy},
             {"mid_energy", p.frequencyBalance.midEnergy},
             {"high_mid_energy", p.frequencyBalance.highMidEnergy},
             {"presence_energy", p.frequencyBalance.presenceEnergy},
             {"air_energy", p.frequencyBalance.airEnergy},
         }},
    };
}

ReferenceProfile fromJson(const nlohmann::json& j)
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

bool ProfileCache::save(const std::filesystem::path& path,
                        const std::vector<ReferenceProfile>& profiles) const
{
    std::filesystem::create_directories(path.parent_path());

    nlohmann::json j = nlohmann::json::array();
    for (const auto& profile : profiles)
    {
        j.push_back(toJson(profile));
    }

    std::ofstream output(path);
    if (!output)
    {
        return false;
    }

    output << j.dump(2);
    return true;
}

std::vector<ReferenceProfile> ProfileCache::load(const std::filesystem::path& path) const
{
    std::vector<ReferenceProfile> profiles;
    std::ifstream input(path);
    if (!input)
    {
        return profiles;
    }

    nlohmann::json j;
    input >> j;
    if (!j.is_array())
    {
        return profiles;
    }

    for (const auto& item : j)
    {
        profiles.push_back(fromJson(item));
    }
    return profiles;
}

} // namespace dawai::reference_engine
