#include "AnalysisEngine.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <limits>
#include <vector>

namespace audiosynth::dsp
{

namespace
{
float finiteOr(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

std::filesystem::path canonicalReferenceManifestPath()
{
    if (const char* envPath = std::getenv("CORESYNTH_REFERENCE_POOL_MANIFEST");
        envPath != nullptr && *envPath != '\0')
    {
        return envPath;
    }

    if (const char* envDir = std::getenv("CORESYNTH_REFERENCE_PROFILE_DIR");
        envDir != nullptr && *envDir != '\0')
    {
        const auto path = std::filesystem::path(envDir);
        if (path.extension() == ".json")
        {
            return path;
        }
        return path.parent_path() / "pool_manifest.json";
    }

    const std::filesystem::path preferredManifest{
        "assets/reference_pools/pro_25x6/pool_manifest.json"};
    if (std::filesystem::exists(preferredManifest))
    {
        return preferredManifest;
    }

    return std::filesystem::path("assets/reference_pools/canonical_25x5/pool_manifest.json");
}

FrequencyBalanceTarget buildFrequencyBalanceTarget(float subBandDb, float lowBandDb,
                                                   float lowMidBandDb, float midBandDb,
                                                   float highMidBandDb, float presenceBandDb,
                                                   float airBandDb)
{
    const auto dbToMagnitude = [](float db)
    {
        if (!std::isfinite(db) || db <= -140.0f)
        {
            return 0.0f;
        }
        return std::pow(10.0f, db / 20.0f);
    };

    const float sub = dbToMagnitude(subBandDb);
    const float bass = dbToMagnitude(lowBandDb);
    const float lowMid = dbToMagnitude(lowMidBandDb);
    const float mid = dbToMagnitude(midBandDb);
    const float highMid = dbToMagnitude(highMidBandDb);
    const float presence = dbToMagnitude(presenceBandDb);
    const float air = dbToMagnitude(airBandDb);
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

float frequencyOrMissing(float value)
{
    return (std::isfinite(value) && value >= 0.0f && value <= 1.0f) ? value : -90.0f;
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
    frame.transientRateHz = juce::jmax(0.0f, finiteOr(frame.transientRateHz, 0.0f));
}

bool inputIsActive(const FeatureFrame& frame)
{
    return frame.peak > 1.0e-4f || frame.rms > 5.0e-5f;
}

void applyStateDefaults(AnalysisFrame& out, double tSec, AnalysisStateCode state,
                        ReferenceStateCode referenceState, bool liveSignalPresent,
                        bool referenceValid, bool scoredResultValid)
{
    out = AnalysisFrame{};
    out.tSec = tSec;
    out.analysisStateCode = state;
    out.referenceStateCode = referenceState;
    out.liveSignalPresent = liveSignalPresent ? 1 : 0;
    out.referenceValid = referenceValid ? 1 : 0;
    out.scoredResultValid = scoredResultValid ? 1 : 0;
}

void sanitizeLoudnessFrame(LoudnessFrame& frame)
{
    frame.integratedLufs = finiteOr(frame.integratedLufs, -70.0f);
    frame.shortTermLufs = finiteOr(frame.shortTermLufs, -70.0f);
    frame.momentaryLufs = finiteOr(frame.momentaryLufs, -70.0f);
    frame.truePeakDbTP = finiteOr(frame.truePeakDbTP, -120.0f);
    frame.peakDbFS = finiteOr(frame.peakDbFS, -120.0f);
}

bool usableReferenceBand(float value)
{
    return std::isfinite(value) && value > -80.0f;
}

float deltaOrMissing(float measured, float target)
{
    if (!usableReferenceBand(measured) || !usableReferenceBand(target))
    {
        return std::numeric_limits<float>::quiet_NaN();
    }
    return measured - target;
}

struct VolatilityBreakdown
{
    float shortIndex = 0.0f;
    float mediumIndex = 0.0f;
    float loudness = 0.0f;
    float spectral = 0.0f;
    float stereo = 0.0f;
    float dynamics = 0.0f;
    float overall = 0.0f;
};

struct BehaviorSummary
{
    float loudnessShortAvg = -99.0f;
    float loudnessMediumAvg = -99.0f;
    float loudnessSessionAvg = -99.0f;
    float loudnessTrend = 0.0f;
    float crestShortAvg = 0.0f;
    float crestMediumAvg = 0.0f;
    float crestSessionAvg = 0.0f;
    float crestTrend = 0.0f;
    float widthShortAvg = 0.0f;
    float widthMediumAvg = 0.0f;
    float widthSessionAvg = 0.0f;
    float correlationShortAvg = 1.0f;
    float correlationMediumAvg = 1.0f;
    float correlationSessionAvg = 1.0f;
    float transientShortAvg = 0.0f;
    float transientMediumAvg = 0.0f;
    float transientSessionAvg = 0.0f;
    float tonalShortAvg = 0.0f;
    float tonalMediumAvg = 0.0f;
    float tonalSessionAvg = 0.0f;
    float volatilityShort = 0.0f;
    float volatilityMedium = 0.0f;
    float loudnessVolatility = 0.0f;
    float spectralVolatility = 0.0f;
    float stereoVolatility = 0.0f;
    float dynamicsVolatility = 0.0f;
    float dynOpen = 0.0f;
    float dynClose = 0.0f;
    float dynHigh = 0.0f;
    float dynLow = 0.0f;
    uint8_t idleUiState = 0;
};

void populateMeasurementFrame(AnalysisFrame& out, const SpectralFrame& spectral,
                              const FeatureFrame& features, const LoudnessFrame& loudness,
                              const StereoFieldFrame& stereoField, float tilt,
                              float dynamicRangeDb, float volatilityIndex)
{
    out.integratedLUFS = loudness.integratedLufs;
    out.shortTermLUFS = loudness.shortTermLufs;
    out.momentaryLUFS = loudness.momentaryLufs;
    out.truePeakDbTP = loudness.truePeakDbTP;
    out.peakDbFS = loudness.peakDbFS;
    out.crestFactorDb = features.crestDb;
    out.dynamicRangeDb = dynamicRangeDb;
    out.stereoWidth = stereoField.width;
    out.spectralBalance = finiteOr(tilt, 0.0f);
    out.transientDensity = features.transientDensity;
    out.transientRateHz = features.transientRateHz;
    out.volatilityIndex = volatilityIndex;

    out.subBandDb = spectral.subBandDb;
    out.lowBandDb = spectral.lowBandDb;
    out.lowMidBandDb = spectral.lowMidBandDb;
    out.midBandDb = spectral.midBandDb;
    out.highMidBandDb = spectral.highMidBandDb;
    out.highBandDb = spectral.highBandDb;
    out.presenceBandDb = spectral.presenceBandDb;
    out.airBandDb = spectral.airBandDb;

    out.midEnergy = stereoField.midEnergy;
    out.sideEnergy = stereoField.sideEnergy;
    out.leftEnergy = stereoField.leftEnergy;
    out.rightEnergy = stereoField.rightEnergy;
    out.correlation = stereoField.correlation;
    out.subMonoIntegrity = stereoField.subMonoIntegrity;
    out.phaseRisk = stereoField.phaseRisk;
    out.lowBandCorrelation = stereoField.lowBandCorrelation;
    out.lowBandPhaseRisk = stereoField.lowBandPhaseRisk;
    out.centerDominance = stereoField.centerDominance;
    out.sideDominance = stereoField.sideDominance;
    out.stereoMotion = stereoField.stereoMotion;
    out.spatialSpread = stereoField.spatialSpread;
    out.spatialW = stereoField.wAxis;
    out.spatialX = stereoField.xAxis;
    out.spatialY = stereoField.yAxis;
    out.spatialZ = stereoField.zAxis;
}

void preprocessMeasurementBuffer(const juce::AudioBuffer<float>& input,
                                 juce::AudioBuffer<float>& output)
{
    const int channels = input.getNumChannels();
    const int samples = input.getNumSamples();
    output.setSize(channels, samples, false, false, true);

    for (int channel = 0; channel < channels; ++channel)
    {
        const float* source = input.getReadPointer(channel);
        float* target = output.getWritePointer(channel);

        double mean = 0.0;
        for (int sample = 0; sample < samples; ++sample)
        {
            mean += static_cast<double>(source[sample]);
        }
        mean /= static_cast<double>(juce::jmax(1, samples));

        for (int sample = 0; sample < samples; ++sample)
        {
            target[sample] = source[sample] - static_cast<float>(mean);
        }
    }
}

float varianceNormalised(const std::deque<VolatilityPoint>& history, double nowSec,
                         double horizonSec, auto valueFn, float referenceScale)
{
    std::vector<float> samples;
    samples.reserve(history.size());
    for (const auto& point : history)
    {
        if (point.tSec >= (nowSec - horizonSec))
        {
            samples.push_back(valueFn(point));
        }
    }

    if (samples.size() < 2)
    {
        return 0.0f;
    }

    float mean = 0.0f;
    for (const auto sample : samples)
    {
        mean += sample;
    }
    mean /= static_cast<float>(samples.size());

    float variance = 0.0f;
    for (const auto sample : samples)
    {
        const float delta = sample - mean;
        variance += delta * delta;
    }
    variance /= static_cast<float>(samples.size() - 1);
    return clamp01(std::sqrt(std::max(0.0f, variance)) / referenceScale);
}

template <typename ValueFn>
float meanWindow(const std::deque<VolatilityPoint>& history, double nowSec, double horizonSec,
                 ValueFn valueFn, float fallback)
{
    float sum = 0.0f;
    int count = 0;
    for (const auto& point : history)
    {
        if (point.tSec < (nowSec - horizonSec))
        {
            continue;
        }

        const float value = valueFn(point);
        if (!std::isfinite(value))
        {
            continue;
        }

        sum += value;
        ++count;
    }

    return count > 0 ? (sum / static_cast<float>(count)) : fallback;
}

VolatilityBreakdown computeVolatilityBreakdown(const std::deque<VolatilityPoint>& history,
                                               double tSec)
{
    VolatilityBreakdown out;

    const float shortLufs =
        varianceNormalised(history, tSec, 1.5, [](const auto& point) { return point.momentaryLufs; },
                           2.6f);
    const float mediumLufs =
        varianceNormalised(history, tSec, 8.0, [](const auto& point) { return point.shortTermLufs; },
                           3.2f);
    const float shortCrest =
        varianceNormalised(history, tSec, 1.5, [](const auto& point) { return point.crestDb; }, 2.2f);
    const float mediumCrest =
        varianceNormalised(history, tSec, 8.0, [](const auto& point) { return point.crestDb; }, 2.8f);
    const float shortTilt = varianceNormalised(history, tSec, 1.5,
                                               [](const auto& point) { return point.spectralTiltDb; },
                                               2.2f);
    const float mediumTilt = varianceNormalised(history, tSec, 8.0,
                                                [](const auto& point) { return point.spectralTiltDb; },
                                                3.4f);
    const float shortWidth =
        varianceNormalised(history, tSec, 1.5, [](const auto& point) { return point.width; }, 0.10f);
    const float mediumWidth =
        varianceNormalised(history, tSec, 8.0, [](const auto& point) { return point.width; }, 0.16f);
    const float shortCorrelation = varianceNormalised(
        history, tSec, 1.5, [](const auto& point) { return point.correlation; }, 0.12f);
    const float mediumCorrelation = varianceNormalised(
        history, tSec, 8.0, [](const auto& point) { return point.correlation; }, 0.18f);
    const float shortTransient = varianceNormalised(
        history, tSec, 1.5, [](const auto& point) { return point.transientDensity; }, 0.12f);
    const float mediumTransient = varianceNormalised(
        history, tSec, 8.0, [](const auto& point) { return point.transientDensity; }, 0.18f);

    out.loudness = clamp01(0.58f * shortLufs + 0.42f * mediumLufs);
    out.spectral = clamp01(0.56f * shortTilt + 0.44f * mediumTilt);
    out.stereo = clamp01(0.28f * shortWidth + 0.22f * mediumWidth + 0.28f * shortCorrelation +
                         0.22f * mediumCorrelation);
    out.dynamics = clamp01(0.36f * shortCrest + 0.24f * mediumCrest + 0.24f * shortTransient +
                           0.16f * mediumTransient);
    out.shortIndex = clamp01(0.28f * shortLufs + 0.24f * shortCrest + 0.18f * shortTilt +
                             0.15f * shortWidth + 0.15f * shortCorrelation);
    out.mediumIndex = clamp01(0.30f * mediumLufs + 0.22f * mediumCrest + 0.18f * mediumTilt +
                              0.15f * mediumWidth + 0.15f * mediumCorrelation);
    out.overall = clamp01(0.58f * out.shortIndex + 0.42f * out.mediumIndex);
    return out;
}

BehaviorSummary computeBehaviorSummary(const std::deque<VolatilityPoint>& history, double tSec,
                                       const LoudnessFrame& loudness, const FeatureFrame& features,
                                       const StereoFieldFrame& stereoField, float spectralTiltDb,
                                       const VolatilityBreakdown& volatility, float microdynamicIndex,
                                       float compressionRisk, int sessionSamples,
                                       float sessionShortTermLufsSum, float sessionCrestDbSum,
                                       float sessionWidthSum, float sessionCorrelationSum,
                                       float sessionTransientDensitySum,
                                       float sessionSpectralTiltSum)
{
    BehaviorSummary out;

    out.loudnessShortAvg =
        meanWindow(history, tSec, 1.5, [](const auto& point) { return point.momentaryLufs; },
                   loudness.momentaryLufs);
    out.loudnessMediumAvg =
        meanWindow(history, tSec, 8.0, [](const auto& point) { return point.shortTermLufs; },
                   loudness.shortTermLufs);
    out.loudnessSessionAvg =
        sessionSamples > 0 ? (sessionShortTermLufsSum / static_cast<float>(sessionSamples))
                           : loudness.shortTermLufs;
    out.loudnessTrend = out.loudnessShortAvg - out.loudnessMediumAvg;

    out.crestShortAvg =
        meanWindow(history, tSec, 1.5, [](const auto& point) { return point.crestDb; },
                   features.crestDb);
    out.crestMediumAvg =
        meanWindow(history, tSec, 8.0, [](const auto& point) { return point.crestDb; },
                   features.crestDb);
    out.crestSessionAvg =
        sessionSamples > 0 ? (sessionCrestDbSum / static_cast<float>(sessionSamples))
                           : features.crestDb;
    out.crestTrend = out.crestShortAvg - out.crestMediumAvg;

    out.widthShortAvg =
        meanWindow(history, tSec, 1.5, [](const auto& point) { return point.width; },
                   stereoField.width);
    out.widthMediumAvg =
        meanWindow(history, tSec, 8.0, [](const auto& point) { return point.width; },
                   stereoField.width);
    out.widthSessionAvg =
        sessionSamples > 0 ? (sessionWidthSum / static_cast<float>(sessionSamples))
                           : stereoField.width;

    out.correlationShortAvg =
        meanWindow(history, tSec, 1.5, [](const auto& point) { return point.correlation; },
                   stereoField.correlation);
    out.correlationMediumAvg =
        meanWindow(history, tSec, 8.0, [](const auto& point) { return point.correlation; },
                   stereoField.correlation);
    out.correlationSessionAvg =
        sessionSamples > 0 ? (sessionCorrelationSum / static_cast<float>(sessionSamples))
                           : stereoField.correlation;

    out.transientShortAvg =
        meanWindow(history, tSec, 1.5, [](const auto& point) { return point.transientDensity; },
                   features.transientDensity);
    out.transientMediumAvg =
        meanWindow(history, tSec, 8.0, [](const auto& point) { return point.transientDensity; },
                   features.transientDensity);
    out.transientSessionAvg =
        sessionSamples > 0 ? (sessionTransientDensitySum / static_cast<float>(sessionSamples))
                           : features.transientDensity;

    out.tonalShortAvg =
        meanWindow(history, tSec, 1.5, [](const auto& point) { return point.spectralTiltDb; },
                   spectralTiltDb);
    out.tonalMediumAvg =
        meanWindow(history, tSec, 8.0, [](const auto& point) { return point.spectralTiltDb; },
                   spectralTiltDb);
    out.tonalSessionAvg =
        sessionSamples > 0 ? (sessionSpectralTiltSum / static_cast<float>(sessionSamples))
                           : spectralTiltDb;

    out.volatilityShort = volatility.shortIndex;
    out.volatilityMedium = volatility.mediumIndex;
    out.loudnessVolatility = volatility.loudness;
    out.spectralVolatility = volatility.spectral;
    out.stereoVolatility = volatility.stereo;
    out.dynamicsVolatility = volatility.dynamics;

    const float sustainStability =
        1.0f - clamp01(std::abs(loudness.momentaryLufs - out.loudnessMediumAvg) / 5.0f);
    const float peakExcursionDb = loudness.truePeakDbTP - out.loudnessShortAvg;

    out.dynOpen = clamp01(0.52f * microdynamicIndex + 0.48f * out.transientShortAvg);
    out.dynClose =
        clamp01(0.58f * compressionRisk + 0.42f * (1.0f - sustainStability));
    out.dynHigh = clamp01((peakExcursionDb - 6.0f) / 10.0f);
    out.dynLow = clamp01(0.62f * sustainStability + 0.38f * (1.0f - volatility.loudness));
    return out;
}

void syncStructuredFrames(AnalysisFrame& out, const BehaviorSummary& behavior)
{
    out.measurement.integratedLUFS = out.integratedLUFS;
    out.measurement.shortTermLUFS = out.shortTermLUFS;
    out.measurement.momentaryLUFS = out.momentaryLUFS;
    out.measurement.truePeakDbTP = out.truePeakDbTP;
    out.measurement.peakDbFS = out.peakDbFS;
    out.measurement.crestFactorDb = out.crestFactorDb;
    out.measurement.dynamicRangeDb = out.dynamicRangeDb;
    out.measurement.stereoWidth = out.stereoWidth;
    out.measurement.correlation = out.correlation;
    out.measurement.phaseRisk = out.phaseRisk;
    out.measurement.transientDensity = out.transientDensity;
    out.measurement.transientRateHz = out.transientRateHz;
    out.measurement.spectralBalance = out.spectralBalance;
    out.measurement.spectralBandsDb = {out.subBandDb, out.lowBandDb, out.lowMidBandDb, out.midBandDb,
                                       out.highMidBandDb, out.highBandDb, out.presenceBandDb,
                                       out.airBandDb};

    out.deviation.loudnessDeltaLUFS =
        std::isfinite(out.integratedLoudnessReferenceLUFS) &&
                out.integratedLoudnessReferenceLUFS > -80.0f
            ? (out.integratedLUFS - out.integratedLoudnessReferenceLUFS)
            : std::numeric_limits<float>::quiet_NaN();
    out.deviation.truePeakDeltaDb = out.truePeakVsReferenceDb;
    out.deviation.dynamicRangeDeltaDb =
        std::isfinite(out.dynamicRangeReferenceDb) && out.dynamicRangeReferenceDb > 0.0f
            ? (out.dynamicRangeDb - out.dynamicRangeReferenceDb)
            : std::numeric_limits<float>::quiet_NaN();
    out.deviation.widthDelta =
        out.targetStereoWidth > 0.0f ? (out.stereoWidth - out.targetStereoWidth)
                                     : std::numeric_limits<float>::quiet_NaN();
    out.deviation.correlationDelta =
        std::abs(out.targetCorrelation) > 0.001f ? (out.correlation - out.targetCorrelation)
                                                 : std::numeric_limits<float>::quiet_NaN();
    out.deviation.spectralTiltDeltaDb =
        std::isfinite(out.referenceMidBandDb) && usableReferenceBand(out.referenceMidBandDb)
            ? (out.spectralBalance - (out.referenceHighMidBandDb - out.referenceLowMidBandDb))
            : std::numeric_limits<float>::quiet_NaN();
    out.deviation.targetWindowDb = out.targetWindowDb;
    out.deviation.bandDeltaDb = {out.subBandDeltaDb, out.lowBandDeltaDb, out.lowMidBandDeltaDb,
                                 out.midBandDeltaDb, out.highMidBandDeltaDb, out.highBandDeltaDb,
                                 out.presenceBandDeltaDb, out.airBandDeltaDb};

    out.behavior.loudnessShortAvg = behavior.loudnessShortAvg;
    out.behavior.loudnessMediumAvg = behavior.loudnessMediumAvg;
    out.behavior.loudnessSessionAvg = behavior.loudnessSessionAvg;
    out.behavior.loudnessTrend = behavior.loudnessTrend;
    out.behavior.crestShortAvg = behavior.crestShortAvg;
    out.behavior.crestMediumAvg = behavior.crestMediumAvg;
    out.behavior.crestSessionAvg = behavior.crestSessionAvg;
    out.behavior.crestTrend = behavior.crestTrend;
    out.behavior.widthShortAvg = behavior.widthShortAvg;
    out.behavior.widthMediumAvg = behavior.widthMediumAvg;
    out.behavior.widthSessionAvg = behavior.widthSessionAvg;
    out.behavior.correlationShortAvg = behavior.correlationShortAvg;
    out.behavior.correlationMediumAvg = behavior.correlationMediumAvg;
    out.behavior.correlationSessionAvg = behavior.correlationSessionAvg;
    out.behavior.transientShortAvg = behavior.transientShortAvg;
    out.behavior.transientMediumAvg = behavior.transientMediumAvg;
    out.behavior.transientSessionAvg = behavior.transientSessionAvg;
    out.behavior.tonalShortAvg = behavior.tonalShortAvg;
    out.behavior.tonalMediumAvg = behavior.tonalMediumAvg;
    out.behavior.tonalSessionAvg = behavior.tonalSessionAvg;
    out.behavior.volatilityShort = behavior.volatilityShort;
    out.behavior.volatilityMedium = behavior.volatilityMedium;
    out.behavior.loudnessVolatility = behavior.loudnessVolatility;
    out.behavior.spectralVolatility = behavior.spectralVolatility;
    out.behavior.stereoVolatility = behavior.stereoVolatility;
    out.behavior.dynamicsVolatility = behavior.dynamicsVolatility;
    out.behavior.dynOpen = behavior.dynOpen;
    out.behavior.dynClose = behavior.dynClose;
    out.behavior.dynHigh = behavior.dynHigh;
    out.behavior.dynLow = behavior.dynLow;
    out.behavior.idleUiState = behavior.idleUiState;

    out.dynOpen = behavior.dynOpen;
    out.dynClose = behavior.dynClose;
    out.dynHigh = behavior.dynHigh;
    out.dynLow = behavior.dynLow;
}

} // namespace

void AnalysisEngine::prepare(double sampleRate, int blockSize)
{
    m_featureExtractor.prepare(sampleRate, blockSize);
    m_loudness.reset(sampleRate, blockSize, 2);
    m_spectrum.prepare(sampleRate);
    m_stereo.prepare(sampleRate);
    m_lastGenre = GenreId::Unknown;
    m_lastGenreConfidence = 0.0f;
    m_activeSignalFrames = 0;
    m_measurementBuffer.setSize(2, juce::jmax(64, blockSize), false, false, true);
    m_volatilityHistory.clear();
    m_sessionSamples = 0;
    m_sessionShortTermLufsSum = 0.0f;
    m_sessionCrestDbSum = 0.0f;
    m_sessionWidthSum = 0.0f;
    m_sessionCorrelationSum = 0.0f;
    m_sessionTransientDensitySum = 0.0f;
    m_sessionSpectralTiltSum = 0.0f;

    (void)m_reference.loadCanonicalPool(canonicalReferenceManifestPath());
    (void)m_reference.loadMetricConfig("assets/reference_intake/analysis_metric_catalog.json");
}

bool AnalysisEngine::process(const juce::AudioBuffer<float>& buffer, double tSec,
                             AnalysisFrame& out)
{
    constexpr int kWarmupFrames = 6;
    constexpr int kScoredFrames = 20;

    preprocessMeasurementBuffer(buffer, m_measurementBuffer);

    auto spectral = m_spectrum.analyze(m_measurementBuffer);
    auto features = m_featureExtractor.extract(m_measurementBuffer, &spectral);
    sanitizeFeatureFrame(features);
    if (!inputIsActive(features))
    {
        m_activeSignalFrames = 0;
        m_volatilityHistory.clear();
        m_sessionSamples = 0;
        m_sessionShortTermLufsSum = 0.0f;
        m_sessionCrestDbSum = 0.0f;
        m_sessionWidthSum = 0.0f;
        m_sessionCorrelationSum = 0.0f;
        m_sessionTransientDensitySum = 0.0f;
        m_sessionSpectralTiltSum = 0.0f;
        applyStateDefaults(out, tSec, kAnalysisStateIdle,
                           m_reference.referenceBlocked() ? kReferenceStateBlocked
                                                         : (m_reference.referenceReady()
                                                                ? kReferenceStateReady
                                                                : kReferenceStateIncomplete),
                           false, m_reference.referenceReady(), false);
        out.behavior.idleUiState = 1;
        out.genreIndex = static_cast<uint8_t>(m_lastGenre);
        out.genreConfidence = m_lastGenreConfidence;
        return true;
    }

    ++m_activeSignalFrames;

    const auto stereoField = m_stereo.analyze(m_measurementBuffer);
    features.midEnergy = stereoField.midEnergy;
    features.sideEnergy = stereoField.sideEnergy;
    features.correlation = stereoField.correlation;

    auto loudness = m_loudness.process(m_measurementBuffer);
    sanitizeLoudnessFrame(loudness);

    m_volatilityHistory.push_back({tSec,
                                   loudness.momentaryLufs,
                                   loudness.shortTermLufs,
                                   features.crestDb,
                                   spectral.spectralTiltDb,
                                   stereoField.width,
                                   stereoField.correlation,
                                   features.transientDensity,
                                   loudness.truePeakDbTP});
    while (!m_volatilityHistory.empty() && m_volatilityHistory.front().tSec < (tSec - 12.0))
    {
        m_volatilityHistory.pop_front();
    }
    const auto volatility = computeVolatilityBreakdown(m_volatilityHistory, tSec);
    const float volatilityIndex = volatility.overall;

    jassert(std::isfinite(features.rms));
    jassert(std::isfinite(features.peak));
    jassert(std::isfinite(features.crestDb));
    jassert(std::isfinite(loudness.integratedLufs));
    jassert(std::isfinite(loudness.truePeakDbTP));

    const auto tilt = spectral.spectralTiltDb;
    const float dynamicRangeDb = juce::jmax(0.0f, loudness.truePeakDbTP - loudness.shortTermLufs);

    const auto referenceState =
        m_reference.referenceBlocked() ? kReferenceStateBlocked
        : m_reference.referenceReady() ? kReferenceStateReady
                                       : kReferenceStateIncomplete;

    const AnalysisStateCode analysisState =
        m_activeSignalFrames < kWarmupFrames
            ? kAnalysisStateBufferWarming
            : (m_activeSignalFrames < kScoredFrames ? kAnalysisStateActiveLive
                                                    : kAnalysisStateValidScored);
    const auto currentFrequencyBalance = buildFrequencyBalanceTarget(
        spectral.subBandDb, spectral.lowBandDb, spectral.lowMidBandDb, spectral.midBandDb,
        spectral.highMidBandDb, spectral.presenceBandDb, spectral.airBandDb);
    const auto frequencyBalanceClassification =
        m_reference.classifyFrequencyBalance(currentFrequencyBalance);
    const auto loudnessClassification =
        m_reference.classifyIntegratedLoudness(loudness.integratedLufs);
    const auto dynamicRangeClassification = m_reference.classifyDynamicRange(dynamicRangeDb);

    const float phaseRisk = juce::jmax(stereoField.phaseRisk, stereoField.lowBandPhaseRisk * 0.85f);
    const float subMonoIntegrity = stereoField.subMonoIntegrity;

    const float harshness = features.harshness;
    const float mud = features.mud;
    const float air = features.air;
    const float compressionRisk =
        m_dynamics.compressionCollapseRisk(features, loudness.truePeakDbTP);
    const float microdynamicIndex = m_dynamics.microdynamicIndex(features);
    if (m_pinnedGenre != GenreId::Unknown)
    {
        m_lastGenre = m_pinnedGenre;
        m_lastGenreConfidence = 1.0f;
    }

    ++m_sessionSamples;
    m_sessionShortTermLufsSum += loudness.shortTermLufs;
    m_sessionCrestDbSum += features.crestDb;
    m_sessionWidthSum += stereoField.width;
    m_sessionCorrelationSum += stereoField.correlation;
    m_sessionTransientDensitySum += features.transientDensity;
    m_sessionSpectralTiltSum += tilt;

    if (referenceState == kReferenceStateReady)
    {
        float detectedConfidence = 0.0f;
        const auto detectedProfile = (m_pinnedGenre != GenreId::Unknown)
                                         ? m_reference.matchGenre(m_pinnedGenre, features, spectral,
                                                                  detectedConfidence)
                                         : m_reference.detectGenre(features, spectral,
                                                                   detectedConfidence);
        const auto stableSwitch = detectedConfidence > (m_lastGenreConfidence + 0.05f) ||
                                  m_lastGenre == GenreId::Unknown ||
                                  detectedProfile.id == m_lastGenre;
        if (stableSwitch)
        {
            m_lastGenre = detectedProfile.id;
            m_lastGenreConfidence = detectedConfidence;
            m_reference.setActiveProfile(detectedProfile);
        }
    }

    const auto& activeProfile = m_reference.activeProfile();
    const auto& referenceMatch = activeProfile.match;
    const auto& referenceFeature = referenceMatch.feature;
    const bool referenceValid = (referenceState == kReferenceStateReady) &&
                                m_reference.activeProfileUsable();
    const bool scoredResultValid =
        (analysisState == kAnalysisStateValidScored) && referenceValid;

    applyStateDefaults(out, tSec, analysisState, referenceState, true, referenceValid,
                       scoredResultValid);
    populateMeasurementFrame(out, spectral, features, loudness, stereoField, tilt, dynamicRangeDb,
                             volatilityIndex);

    out.integratedLoudnessReferenceLUFS = loudnessClassification.referenceLufs;
    out.dynamicRangeReferenceDb = dynamicRangeClassification.referenceDb;
    out.dynamicRangeStatusCode = dynamicRangeClassification.statusCode;
    out.dynamicRangeSeverity = dynamicRangeClassification.severity;
    out.targetWindowDb = 3.0f;
    out.frequencySubEnergy = frequencyOrMissing(currentFrequencyBalance.sub);
    out.frequencyBassEnergy = frequencyOrMissing(currentFrequencyBalance.bass);
    out.frequencyLowMidEnergy = frequencyOrMissing(currentFrequencyBalance.lowMid);
    out.frequencyMidEnergy = frequencyOrMissing(currentFrequencyBalance.mid);
    out.frequencyHighMidEnergy = frequencyOrMissing(currentFrequencyBalance.highMid);
    out.frequencyPresenceEnergy = frequencyOrMissing(currentFrequencyBalance.presence);
    out.frequencyAirEnergy = frequencyOrMissing(currentFrequencyBalance.air);
    out.frequencyBalanceStatusCode = frequencyBalanceClassification.statusCode;
    out.frequencyBalanceSeverity = frequencyBalanceClassification.severity;
    out.integratedLoudnessStatusCode = loudnessClassification.statusCode;
    out.integratedLoudnessSeverity = loudnessClassification.severity;

    out.phaseRisk = phaseRisk;
    out.genreIndex = static_cast<uint8_t>(m_lastGenre);
    out.genreConfidence = m_lastGenreConfidence;

    out.flags = kFlagNone;

    if (referenceValid)
    {
        out.referenceTruePeakDbTP = activeProfile.match.truePeakDbTP;
        out.targetTruePeakDbTP = activeProfile.mean.truePeakDbTP;
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
        out.targetFrequencySubEnergy = frequencyOrMissing(activeProfile.mean.frequencyBalance.sub);
        out.targetFrequencyBassEnergy = frequencyOrMissing(activeProfile.mean.frequencyBalance.bass);
        out.targetFrequencyLowMidEnergy =
            frequencyOrMissing(activeProfile.mean.frequencyBalance.lowMid);
        out.targetFrequencyMidEnergy = frequencyOrMissing(activeProfile.mean.frequencyBalance.mid);
        out.targetFrequencyHighMidEnergy =
            frequencyOrMissing(activeProfile.mean.frequencyBalance.highMid);
        out.targetFrequencyPresenceEnergy =
            frequencyOrMissing(activeProfile.mean.frequencyBalance.presence);
        out.targetFrequencyAirEnergy = frequencyOrMissing(activeProfile.mean.frequencyBalance.air);
        out.subBandDeltaDb = deltaOrMissing(out.subBandDb, out.targetSubBandDb);
        out.lowBandDeltaDb = deltaOrMissing(out.lowBandDb, out.targetLowBandDb);
        out.lowMidBandDeltaDb = deltaOrMissing(out.lowMidBandDb, out.targetLowMidBandDb);
        out.midBandDeltaDb = deltaOrMissing(out.midBandDb, out.targetMidBandDb);
        out.highMidBandDeltaDb = deltaOrMissing(out.highMidBandDb, out.targetHighMidBandDb);
        out.highBandDeltaDb = deltaOrMissing(out.highBandDb, out.targetHighBandDb);
        out.presenceBandDeltaDb =
            deltaOrMissing(out.presenceBandDb, out.targetPresenceBandDb);
        out.airBandDeltaDb = deltaOrMissing(out.airBandDb, out.targetAirBandDb);
        out.referenceStereoWidth = clamp01(referenceFeature.sideEnergy);
        out.referenceCorrelation =
            juce::jlimit(-1.0f, 1.0f, finiteOr(referenceFeature.correlation, 0.0f));
        out.targetStereoWidth = clamp01(activeProfile.mean.feature.sideEnergy);
        out.targetCorrelation =
            juce::jlimit(-1.0f, 1.0f, finiteOr(activeProfile.mean.feature.correlation, 0.0f));
        if (out.referenceTruePeakDbTP > -80.0f)
        {
            out.truePeakVsReferenceDb = out.truePeakDbTP - out.referenceTruePeakDbTP;
        }
        else
        {
            out.truePeakVsReferenceDb = std::numeric_limits<float>::quiet_NaN();
        }
    }

    if (scoredResultValid)
    {
        const float transientMatch = m_transient.transientMatch(features, referenceFeature);
        const float stereoMatch = m_stereo.stereoSignatureMatch(features, referenceFeature);
        const auto segment = m_scoring.score(features, spectral, activeProfile.match,
                                             activeProfile.mean, harshness, mud, air,
                                             transientMatch, stereoMatch, subMonoIntegrity,
                                             phaseRisk, compressionRisk, static_cast<float>(tSec));
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
    }

    auto behavior = computeBehaviorSummary(
        m_volatilityHistory, tSec, loudness, features, stereoField, tilt, volatility,
        microdynamicIndex, compressionRisk, m_sessionSamples, m_sessionShortTermLufsSum,
        m_sessionCrestDbSum, m_sessionWidthSum, m_sessionCorrelationSum,
        m_sessionTransientDensitySum, m_sessionSpectralTiltSum);
    behavior.idleUiState = analysisState == kAnalysisStateBufferWarming ? 1 : 0;
    syncStructuredFrames(out, behavior);

    const float widthFloor = referenceValid
                                 ? juce::jmax(0.12f, juce::jmax(out.referenceStereoWidth * 0.55f,
                                                                out.targetStereoWidth * 0.50f))
                                 : 0.18f;

    if (analysisState == kAnalysisStateValidScored &&
        (phaseRisk > 0.35f || out.lowBandPhaseRisk > 0.20f || out.lowBandCorrelation < 0.15f))
        out.flags |= kFlagPhaseRisk;
    if (analysisState == kAnalysisStateValidScored && loudness.truePeakDbTP > -0.3f)
        out.flags |= kFlagClipRisk;
    if (analysisState == kAnalysisStateValidScored &&
        (harshness > 0.65f ||
         (std::isfinite(out.highMidBandDeltaDb) && out.highMidBandDeltaDb > 3.0f) ||
         (std::isfinite(out.highBandDeltaDb) && out.highBandDeltaDb > 4.0f)))
        out.flags |= kFlagHarshnessBurst;
    if (analysisState == kAnalysisStateValidScored &&
        (out.stereoWidth < widthFloor ||
         (out.centerDominance > 0.78f && out.sideDominance < 0.18f)))
        out.flags |= kFlagWidthCollapse;
    if (analysisState == kAnalysisStateValidScored && features.crestDb < 4.0f &&
        features.transientDensity < 0.35f)
        out.flags |= kFlagTransientSoftening;

    m_snapshots.push(out);
    return true;
}

} // namespace audiosynth::dsp
