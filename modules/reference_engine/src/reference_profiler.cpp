#include "dawai/reference_engine/reference_profiler.hpp"

#include <algorithm>
#include <cmath>

namespace dawai::reference_engine
{

namespace
{
std::vector<double> rollingLoudness(const dawai::metering::AudioBlock& block, std::size_t window)
{
    std::vector<double> values;
    if (block.numSamples() == 0 || block.numChannels() == 0 || window == 0)
    {
        return values;
    }

    for (std::size_t i = 0; i + window <= block.numSamples(); i += window)
    {
        dawai::metering::AudioBlock chunk;
        chunk.channels.resize(block.numChannels());
        for (std::size_t c = 0; c < block.numChannels(); ++c)
        {
            chunk.channels[c].assign(block.channels[c].begin() + static_cast<std::ptrdiff_t>(i),
                                     block.channels[c].begin() +
                                         static_cast<std::ptrdiff_t>(i + window));
        }

        dawai::metering::LoudnessAnalyzer analyzer(window);
        values.push_back(analyzer.analyze(chunk).shortTermLufs);
    }

    return values;
}

FrequencyBalanceProfile buildFrequencyBalanceProfile(const ReferenceProfile& profile)
{
    const auto dbToEnergy = [](double db)
    {
        if (!std::isfinite(db) || db <= -140.0)
        {
            return 0.0;
        }
        return std::pow(10.0, db / 10.0);
    };

    const double sub = dbToEnergy(profile.subBandDb);
    const double bass = dbToEnergy(profile.lowBandDb);
    const double lowMid = dbToEnergy(profile.lowMidBandDb);
    const double mid = dbToEnergy(profile.midBandDb);
    const double highMid = dbToEnergy(profile.highMidBandDb);
    const double presence = dbToEnergy(profile.presenceBandDb);
    const double air = dbToEnergy(profile.airBandDb);
    const double total = sub + bass + lowMid + mid + highMid + presence + air;

    FrequencyBalanceProfile balance;
    if (total <= 0.0)
    {
        return balance;
    }

    balance.subEnergy = sub / total;
    balance.bassEnergy = bass / total;
    balance.lowMidEnergy = lowMid / total;
    balance.midEnergy = mid / total;
    balance.highMidEnergy = highMid / total;
    balance.presenceEnergy = presence / total;
    balance.airEnergy = air / total;
    return balance;
}

} // namespace

ReferenceProfiler::ReferenceProfiler(std::size_t fftSize, std::size_t bands)
    : m_meteringEngine(fftSize, bands)
{
}

ReferenceProfile ReferenceProfiler::buildProfile(const std::string& id, const std::string& title,
                                                 const std::string& sourcePath,
                                                 const dawai::metering::AudioBlock& block,
                                                 double sampleRate)
{
    const auto snapshot = m_meteringEngine.process(block, sampleRate);

    ReferenceProfile profile;
    profile.id = id;
    profile.title = title;
    profile.sourcePath = sourcePath;
    profile.spectrumBandsDb = snapshot.spectrum.averagedBinsDb;
    profile.spectralTiltDb = snapshot.spectrum.spectralTiltDb;
    profile.subBandDb = snapshot.spectrum.lowBandDb;
    profile.lowBandDb = snapshot.spectrum.lowBandDb;
    profile.lowMidBandDb = snapshot.spectrum.lowMidBandDb;
    profile.midBandDb = (snapshot.spectrum.lowMidBandDb + snapshot.spectrum.presenceBandDb) * 0.5;
    profile.highMidBandDb = snapshot.spectrum.presenceBandDb;
    profile.presenceBandDb = snapshot.spectrum.presenceBandDb;
    profile.airBandDb = snapshot.spectrum.airBandDb;
    profile.integratedLufs = snapshot.loudness.integratedLufs;
    profile.shortTermLufs = snapshot.loudness.shortTermLufs;
    profile.truePeakDbtp = snapshot.loudness.truePeakDbtp;
    profile.shortTermLufsDistribution =
        rollingLoudness(block, static_cast<std::size_t>(sampleRate));
    profile.rmsDb = snapshot.dynamics.rmsDb;
    profile.peakDbfs = snapshot.dynamics.peakDbfs;
    profile.crestFactorDb = snapshot.dynamics.crestFactorDb;
    profile.crestFactorDistribution.push_back(snapshot.dynamics.crestFactorDb);
    profile.correlation = snapshot.stereo.correlation;
    profile.stereoWidth = snapshot.stereo.width;
    profile.midEnergy = snapshot.stereo.midEnergy;
    profile.sideEnergy = snapshot.stereo.sideEnergy;
    profile.phaseRisk = snapshot.stereo.phaseRisk;
    profile.subMonoIntegrity = snapshot.stereo.subMonoIntegrity;
    profile.transientDensity = snapshot.dynamics.transientDensity;
    profile.frequencyBalance = buildFrequencyBalanceProfile(profile);

    return profile;
}

ProfileDelta computeDelta(const dawai::metering::MeterSnapshot& mix,
                          const ReferenceProfile& referenceProfile)
{
    ProfileDelta delta;
    const auto bands =
        std::min(mix.spectrum.averagedBinsDb.size(), referenceProfile.spectrumBandsDb.size());
    delta.spectrumDeltaDb.resize(bands, 0.0);

    for (std::size_t i = 0; i < bands; ++i)
    {
        delta.spectrumDeltaDb[i] =
            mix.spectrum.averagedBinsDb[i] - referenceProfile.spectrumBandsDb[i];
    }

    delta.integratedLufsDelta = mix.loudness.integratedLufs - referenceProfile.integratedLufs;
    delta.truePeakDelta = mix.loudness.truePeakDbtp - referenceProfile.truePeakDbtp;
    const double refCrest = referenceProfile.crestFactorDistribution.empty()
                                ? 0.0
                                : referenceProfile.crestFactorDistribution.front();
    delta.crestFactorDelta = mix.dynamics.crestFactorDb - refCrest;
    delta.widthDelta = mix.stereo.width - referenceProfile.stereoWidth;
    return delta;
}

} // namespace dawai::reference_engine
