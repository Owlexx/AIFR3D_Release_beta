#pragma once

#include <string>
#include <vector>

namespace dawai::reference_engine
{

struct FrequencyBalanceProfile
{
    double subEnergy = 0.0;
    double bassEnergy = 0.0;
    double lowMidEnergy = 0.0;
    double midEnergy = 0.0;
    double highMidEnergy = 0.0;
    double presenceEnergy = 0.0;
    double airEnergy = 0.0;
};

struct ReferenceProfile
{
    std::string id;
    std::string title;
    std::string sourcePath;

    std::vector<double> spectrumBandsDb;
    double spectralTiltDb = 0.0;
    double subBandDb = -90.0;
    double lowBandDb = -90.0;
    double lowMidBandDb = -90.0;
    double midBandDb = -90.0;
    double highMidBandDb = -90.0;
    double presenceBandDb = -90.0;
    double airBandDb = -90.0;
    double integratedLufs = -23.0;
    double shortTermLufs = -23.0;
    double truePeakDbtp = -1.0;
    std::vector<double> shortTermLufsDistribution;
    double rmsDb = -70.0;
    double peakDbfs = -120.0;
    double crestFactorDb = 0.0;
    std::vector<double> crestFactorDistribution;
    double correlation = 1.0;
    double stereoWidth = 0.0;
    double midEnergy = 0.0;
    double sideEnergy = 0.0;
    double phaseRisk = 0.0;
    double subMonoIntegrity = 1.0;
    double transientDensity = 0.0;
    FrequencyBalanceProfile frequencyBalance;
};

struct ProfileDelta
{
    std::vector<double> spectrumDeltaDb;
    double integratedLufsDelta = 0.0;
    double truePeakDelta = 0.0;
    double crestFactorDelta = 0.0;
    double widthDelta = 0.0;
};

} // namespace dawai::reference_engine
