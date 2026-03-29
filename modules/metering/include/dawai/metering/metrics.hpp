#pragma once

#include <cstddef>
#include <vector>

namespace dawai::metering
{

struct AudioBlock
{
    std::vector<std::vector<float>> channels;

    [[nodiscard]] std::size_t numChannels() const noexcept
    {
        return channels.size();
    }

    [[nodiscard]] std::size_t numSamples() const noexcept;
};

struct SpectrumMetrics
{
    std::vector<double> averagedBinsDb;
    double binWidthHz = 0.0;
    double spectralTiltDb = 0.0;
    double lowBandDb = -90.0;
    double lowMidBandDb = -90.0;
    double presenceBandDb = -90.0;
    double airBandDb = -90.0;
};

struct LoudnessMetrics
{
    double integratedLufs = -70.0;
    double shortTermLufs = -70.0;
    double truePeakDbtp = -120.0;
};

struct StereoMetrics
{
    double correlation = 1.0;
    double width = 0.0;
    double midEnergy = 0.0;
    double sideEnergy = 0.0;
    double phaseRisk = 0.0;
    double subMonoIntegrity = 1.0;
};

struct DynamicMetrics
{
    double rmsDb = -70.0;
    double peakDbfs = -120.0;
    double crestFactorDb = 0.0;
    double transientDensity = 0.0;
};

struct MeterSnapshot
{
    SpectrumMetrics spectrum;
    LoudnessMetrics loudness;
    StereoMetrics stereo;
    DynamicMetrics dynamics;
};

} // namespace dawai::metering
