#include "AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>

namespace aifred::dsp
{

namespace
{
float finiteOr(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

FrequencyBalanceTarget buildFrequencyBalanceTarget(float subBandDb, float lowBandDb,
                                                   float lowMidBandDb, float midBandDb,
                                                   float highMidBandDb, float presenceBandDb,
                                                   float airBandDb)
{
    const auto dbToEnergy = [](float db)
    {
        if (!std::isfinite(db) || db <= -140.0f)
        {
            return 0.0f;
        }
        return std::pow(10.0f, db / 10.0f);
    };

    const float sub = dbToEnergy(subBandDb);
    const float bass = dbToEnergy(lowBandDb);
    const float lowMid = dbToEnergy(lowMidBandDb);
    const float mid = dbToEnergy(midBandDb);
    const float highMid = dbToEnergy(highMidBandDb);
    const float presence = dbToEnergy(presenceBandDb);
    const float air = dbToEnergy(airBandDb);
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

void sanitizeFeatureFrame(FeatureFrame& frame)
{
    frame.rms = juce::jmax(0.0f, finiteOr(frame.rms, 0.0f));
    frame.peak = juce::jmax(0.0f, finiteOr(frame.peak, 0.0f));
    frame.crestDb = finiteOr(frame.crestDb, 0.0f);
    frame.midEnergy = clamp01(finiteOr(frame.midEnergy, 0.5f));
    frame.sideEnergy = clamp01(finiteOr(frame.sideEnergy, 0.0f));
    frame.correlation = juce::jlimit(-1.0f, 1.0f, finiteOr(frame.correlation, 1.0f));
    frame.harshness = clamp01(finiteOr(frame.harshness, 0.0f));
    frame.mud = clamp01(finiteOr(frame.mud, 0.0f));
    frame.air = clamp01(finiteOr(frame.air, 0.0f));
    frame.transientDensity = clamp01(finiteOr(frame.transientDensity, 0.0f));
}

bool inputIsActive(const FeatureFrame& frame)
{
    return frame.peak > 1.0e-4f || frame.rms > 5.0e-5f;
}

void sanitizeLoudnessFrame(LoudnessFrame& frame)
{
    frame.momentaryLufs = finiteOr(frame.momentaryLufs, -70.0f);
    frame.integratedLufs = finiteOr(frame.integratedLufs, -70.0f);
    frame.shortTermLufs = finiteOr(frame.shortTermLufs, -70.0f);
    frame.truePeakDbTP = finiteOr(frame.truePeakDbTP, -120.0f);
}

} // namespace

void AnalysisEngine::prepare(double sampleRate, int blockSize)
{
    m_featureExtractor.prepare(sampleRate, blockSize);
    m_loudness.reset(sampleRate);
    m_spectrum.prepare(sampleRate);
    m_lastGenre = GenreId::Unknown;
    m_lastGenreConfidence = 0.0f;

    auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto defaultAssetDir = appDataDir.getChildFile("dawai/assets");
    
    // Default to the shared installation path
    std::filesystem::path profileDir = (defaultAssetDir.getChildFile("reference_pools/pro_25x6/profiles")).getFullPathName().toStdString();
    std::filesystem::path metricCatalogPath = (defaultAssetDir.getChildFile("reference_intake/analysis_metric_catalog.json")).getFullPathName().toStdString();

    if (const char* envDir = std::getenv("DAWAI_REFERENCE_PROFILE_DIR"))
    {
        profileDir = envDir;
    }
    else if (!std::filesystem::exists(profileDir))
    {
        // Fallback to relative path if standard location doesn't exist (e.g. during development)
        profileDir = "assets/reference_pools/pro_25x6/profiles";
    }

    (void)m_reference.loadGenreMeansFromProfiles(profileDir);
    if (!m_reference.loadMetricConfig(profileDir.parent_path() / "pool_manifest.json"))
    {
        if (std::filesystem::exists(metricCatalogPath))
        {
            (void)m_reference.loadMetricConfig(metricCatalogPath);
        }
        else
        {
            (void)m_reference.loadMetricConfig("assets/reference_intake/analysis_metric_catalog.json");
        }
    }
}

bool AnalysisEngine::process(const juce::AudioBuffer<float>& buffer, double tSec,
                             AnalysisFrame& out)
{
    const auto spectral = m_spectrum.analyze(buffer);
    auto features = m_featureExtractor.extract(buffer, &spectral);
    sanitizeFeatureFrame(features);
    if (!inputIsActive(features))
    {
        return false;
    }

    auto loudness = m_loudness.process(buffer, tSec);
    sanitizeLoudnessFrame(loudness);

    jassert(std::isfinite(features.rms));
    jassert(std::isfinite(features.peak));
    jassert(std::isfinite(features.crestDb));
    jassert(std::isfinite(loudness.integratedLufs));
    jassert(std::isfinite(loudness.truePeakDbTP));

    const auto tilt = spectral.spectralTiltDb;

    float detectedConfidence = 0.0f;
    const auto detectedProfile = m_reference.detectGenre(features, spectral, detectedConfidence);
    const auto stableSwitch = detectedConfidence > (m_lastGenreConfidence + 0.05f) ||
                              m_lastGenre == GenreId::Unknown || detectedProfile.id == m_lastGenre;
    if (stableSwitch)
    {
        m_lastGenre = detectedProfile.id;
        m_lastGenreConfidence = detectedConfidence;
        m_reference.setActiveProfile(detectedProfile);
    }

    const auto& activeProfile = m_reference.activeProfile();
    const auto& referenceMatch = activeProfile.match;
    const auto& referenceFeature = referenceMatch.feature;
    const auto currentFrequencyBalance = buildFrequencyBalanceTarget(
        spectral.subBandDb, spectral.lowBandDb, spectral.lowMidBandDb, spectral.midBandDb,
        spectral.highMidBandDb, spectral.presenceBandDb, spectral.airBandDb);
    const auto frequencyBalanceClassification =
        m_reference.classifyFrequencyBalance(currentFrequencyBalance);
    const auto loudnessClassification =
        m_reference.classifyIntegratedLoudness(loudness.integratedLufs);
    const auto dynamicRangeClassification = m_reference.classifyDynamicRange(features.crestDb);

    const float transientMatch = m_transient.transientMatch(features, referenceFeature);
    const float stereoMatch = m_stereo.stereoSignatureMatch(features, referenceFeature);
    const float phaseRisk = m_stereo.phaseRisk(features);
    const float subMonoIntegrity = m_stereo.subMonoIntegrity(features);

    const float harshness = features.harshness;
    const float mud = features.mud;
    const float air = features.air;
    const float compressionRisk =
        m_dynamics.compressionCollapseRisk(features, loudness.truePeakDbTP);

    const auto segment = m_scoring.score(features, spectral, activeProfile.match, activeProfile.mean,
                                         harshness, mud, air, transientMatch, stereoMatch,
                                         subMonoIntegrity, phaseRisk, compressionRisk,
                                         static_cast<float>(tSec));

    const float samplePeakDbFS =
        static_cast<float>(20.0 * std::log10(std::max(static_cast<double>(features.peak), 1.0e-12)));
    const float rmsDbFS =
        static_cast<float>(20.0 * std::log10(std::max(static_cast<double>(features.rms), 1.0e-12)));

    out.tSec = tSec;
    out.behaviorIndex = segment.behaviorIndex;
    out.mixAlignment = segment.mixAlignment;
    out.signalClarity = segment.signalClarity;
    out.signalStability = segment.signalStability;
    out.lifeIndex = segment.life;
    out.approval = segment.approval;
    out.confidence = out.signalClarity;

    out.segTone = segment.tone;
    out.segDyn = segment.dynamics;
    out.segSpace = segment.space;
    out.segPunch = segment.punch;
    out.segBalance = segment.balance;

    out.momentaryLUFS = loudness.momentaryLufs;
    out.integratedLUFS = loudness.integratedLufs;
    out.shortTermLUFS = loudness.shortTermLufs;
    out.truePeakDbTP = loudness.truePeakDbTP;
    out.samplePeakDbFS = samplePeakDbFS;
    out.rmsDbFS = rmsDbFS;
    out.integratedLoudnessReferenceLUFS = loudnessClassification.referenceLufs;
    out.referenceTruePeakDbTP = activeProfile.match.truePeakDbTP;
    out.targetTruePeakDbTP = activeProfile.mean.truePeakDbTP;
    out.crestFactorDb = features.crestDb;
    out.dynamicRangeDb = samplePeakDbFS - rmsDbFS;
    out.dynamicRangeReferenceDb = dynamicRangeClassification.referenceDb;
    out.dynamicRangeStatusCode = dynamicRangeClassification.statusCode;
    out.dynamicRangeSeverity = dynamicRangeClassification.severity;
    out.stereoWidth = features.sideEnergy;
    out.spectralBalance = finiteOr(tilt, 0.0f);
    out.transientDensity = features.transientDensity;
    out.subBandDb = spectral.subBandDb;
    out.lowBandDb = spectral.lowBandDb;
    out.lowMidBandDb = spectral.lowMidBandDb;
    out.midBandDb = spectral.midBandDb;
    out.highMidBandDb = spectral.highMidBandDb;
    out.highBandDb = spectral.highBandDb;
    out.presenceBandDb = spectral.presenceBandDb;
    out.airBandDb = spectral.airBandDb;
    out.referenceSubBandDb = activeProfile.match.subBandDb;
    out.referenceLowBandDb = activeProfile.match.lowBandDb;
    out.referenceLowMidBandDb = activeProfile.match.lowMidBandDb;
    out.referenceMidBandDb = activeProfile.match.midBandDb;
    out.referenceHighMidBandDb = activeProfile.match.highMidBandDb;
    out.referenceHighBandDb = activeProfile.match.highBandDb;
    out.referencePresenceBandDb = activeProfile.match.presenceBandDb;
    out.referenceAirBandDb = activeProfile.match.airBandDb;
    out.targetSubBandDb = activeProfile.mean.subBandDb;
    out.targetLowBandDb = activeProfile.mean.lowBandDb;
    out.targetLowMidBandDb = activeProfile.mean.lowMidBandDb;
    out.targetMidBandDb = activeProfile.mean.midBandDb;
    out.targetHighMidBandDb = activeProfile.mean.highMidBandDb;
    out.targetHighBandDb = activeProfile.mean.highBandDb;
    out.targetPresenceBandDb = activeProfile.mean.presenceBandDb;
    out.targetAirBandDb = activeProfile.mean.airBandDb;
    out.targetWindowDb = 3.0f;
    out.frequencySubEnergy = currentFrequencyBalance.sub;
    out.frequencyBassEnergy = currentFrequencyBalance.bass;
    out.frequencyLowMidEnergy = currentFrequencyBalance.lowMid;
    out.frequencyMidEnergy = currentFrequencyBalance.mid;
    out.frequencyHighMidEnergy = currentFrequencyBalance.highMid;
    out.frequencyPresenceEnergy = currentFrequencyBalance.presence;
    out.frequencyAirEnergy = currentFrequencyBalance.air;
    out.targetFrequencySubEnergy = activeProfile.mean.frequencyBalance.sub;
    out.targetFrequencyBassEnergy = activeProfile.mean.frequencyBalance.bass;
    out.targetFrequencyLowMidEnergy = activeProfile.mean.frequencyBalance.lowMid;
    out.targetFrequencyMidEnergy = activeProfile.mean.frequencyBalance.mid;
    out.targetFrequencyHighMidEnergy = activeProfile.mean.frequencyBalance.highMid;
    out.targetFrequencyPresenceEnergy = activeProfile.mean.frequencyBalance.presence;
    out.targetFrequencyAirEnergy = activeProfile.mean.frequencyBalance.air;
    out.frequencyBalanceStatusCode = frequencyBalanceClassification.statusCode;
    out.frequencyBalanceSeverity = frequencyBalanceClassification.severity;
    out.subBandDeltaDb = out.subBandDb - out.referenceSubBandDb;
    out.lowBandDeltaDb = out.lowBandDb - out.referenceLowBandDb;
    out.lowMidBandDeltaDb = out.lowMidBandDb - out.referenceLowMidBandDb;
    out.midBandDeltaDb = out.midBandDb - out.referenceMidBandDb;
    out.highMidBandDeltaDb = out.highMidBandDb - out.referenceHighMidBandDb;
    out.highBandDeltaDb = out.highBandDb - out.referenceHighBandDb;
    out.presenceBandDeltaDb = out.presenceBandDb - out.referencePresenceBandDb;
    out.airBandDeltaDb = out.airBandDb - out.referenceAirBandDb;
    if (out.referenceTruePeakDbTP > -80.0f)
    {
        out.truePeakVsReferenceDb = out.truePeakDbTP - out.referenceTruePeakDbTP;
    }
    else
    {
        out.truePeakVsReferenceDb = 0.0f;
    }
    out.integratedLoudnessStatusCode = loudnessClassification.statusCode;
    out.integratedLoudnessSeverity = loudnessClassification.severity;

    out.midEnergy = features.midEnergy;
    out.sideEnergy = features.sideEnergy;
    out.correlation = features.correlation;
    out.subMonoIntegrity = subMonoIntegrity;
    out.phaseRisk = phaseRisk;

    out.dynOpen = clamp01((features.rms * 1.4f));
    out.dynClose = clamp01((features.rms * 1.2f));
    out.dynHigh = clamp01((features.peak * 1.0f));
    out.dynLow = clamp01((features.rms * 0.9f));
    out.genreIndex = static_cast<uint8_t>(m_lastGenre);
    out.genreConfidence = m_lastGenreConfidence;

    out.flags = kFlagNone;
    if (phaseRisk > 0.35f)
        out.flags |= kFlagPhaseRisk;
    if (loudness.truePeakDbTP > -0.3f)
        out.flags |= kFlagClipRisk;
    if (harshness > 0.65f || out.highMidBandDeltaDb > 3.0f || out.highBandDeltaDb > 4.0f)
        out.flags |= kFlagHarshnessBurst;
    if (features.sideEnergy < 0.2f)
        out.flags |= kFlagWidthCollapse;
    if (features.crestDb < 4.0f && features.transientDensity < 0.35f)
        out.flags |= kFlagTransientSoftening;

    (void)tilt;
    m_snapshots.push(out);
    return true;
}

} // namespace aifred::dsp
