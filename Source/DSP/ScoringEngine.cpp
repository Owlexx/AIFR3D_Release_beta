#include "ScoringEngine.h"
#include "../Common/AnalysisTypes.h"

#include <algorithm>
#include <cmath>

namespace aifred::dsp
{

namespace
{
constexpr float kFeatureTargetEpsilon = 1.0e-4f;

bool hasUsableSpectralValue(float value)
{
    return std::isfinite(value) && value > -80.0f;
}

bool hasUsableFeatureRatio(float value)
{
    return std::isfinite(value) && value > kFeatureTargetEpsilon && value <= 1.0f;
}

bool hasUsableFeatureMagnitude(float value)
{
    return std::isfinite(value) && value > 0.0f;
}

bool hasUsableSignedFeatureRatio(float value)
{
    return std::isfinite(value) && std::abs(value) > kFeatureTargetEpsilon && value >= -1.0f &&
           value <= 1.0f;
}

bool signatureHasUsableFeatureTargets(const ReferenceSignature& signature)
{
    const auto& feature = signature.feature;
    return hasUsableFeatureMagnitude(feature.crestDb) ||
           hasUsableFeatureRatio(feature.transientDensity) ||
           hasUsableFeatureRatio(feature.sideEnergy) || hasUsableFeatureRatio(feature.midEnergy) ||
           hasUsableSignedFeatureRatio(feature.correlation) ||
           hasUsableFeatureRatio(feature.harshness) || hasUsableFeatureRatio(feature.mud) ||
           hasUsableFeatureRatio(feature.air);
}

float selectFeatureTarget(float matched, bool matchedUsable, float fallback, bool fallbackUsable,
                          float safeDefault)
{
    if (matchedUsable && (hasUsableFeatureMagnitude(matched) || hasUsableFeatureRatio(matched) ||
                          hasUsableSignedFeatureRatio(matched)))
    {
        return matched;
    }
    if (fallbackUsable &&
        (hasUsableFeatureMagnitude(fallback) || hasUsableFeatureRatio(fallback) ||
         hasUsableSignedFeatureRatio(fallback)))
    {
        return fallback;
    }
    return safeDefault;
}

float spectralWindowMatch(float current, float target, float reference, float targetWindowDb = 3.0f)
{
    if (!std::isfinite(current) || !std::isfinite(target))
    {
        return 0.0f;
    }

    const float usableReference = std::isfinite(reference) ? reference : target;
    const float targetValue = 0.7f * target + 0.3f * usableReference;
    const float tolerance = std::max(targetWindowDb, 0.001f);
    return clamp01(1.0f - (std::abs(current - targetValue) / (tolerance * 2.0f)));
}

float spectralVectorMatch(const SpectralFrame& spectral, const ReferenceSignature& targetSignature,
                          const ReferenceSignature& fallbackSignature)
{
    if (spectral.averagedBinsDb.empty())
    {
        return 0.0f;
    }

    const auto& targetBands = !targetSignature.spectrumBandsDb.empty() ? targetSignature.spectrumBandsDb
                                                                       : fallbackSignature.spectrumBandsDb;
    if (targetBands.empty())
    {
        return 0.0f;
    }

    const std::size_t count =
        std::min<std::size_t>(spectral.averagedBinsDb.size(), targetBands.size());
    float weightedScore = 0.0f;
    float totalWeight = 0.0f;
    for (std::size_t i = 0; i < count; ++i)
    {
        if (!hasUsableSpectralValue(targetBands[i]))
        {
            continue;
        }

        float bandWeight = 1.0f;
        if (i < count / 4)
        {
            bandWeight = 1.35f;
        }
        else if (i >= (count * 3) / 4)
        {
            bandWeight = 1.15f;
        }

        const float fallbackValue =
            (i < fallbackSignature.spectrumBandsDb.size()) ? fallbackSignature.spectrumBandsDb[i]
                                                           : targetBands[i];
        if (!hasUsableSpectralValue(fallbackValue) && !hasUsableSpectralValue(targetBands[i]))
        {
            continue;
        }
        weightedScore +=
            bandWeight *
            spectralWindowMatch(spectral.averagedBinsDb[i], targetBands[i], fallbackValue, 3.0f);
        totalWeight += bandWeight;
    }

    return (totalWeight > 0.0f) ? (weightedScore / totalWeight) : 0.0f;
}

float usableBandMatch(float current, float target, float reference, float targetWindowDb = 3.0f)
{
    if (!hasUsableSpectralValue(target))
    {
        return 0.5f;
    }

    const float usableReference = hasUsableSpectralValue(reference) ? reference : target;
    return spectralWindowMatch(current, target, usableReference, targetWindowDb);
}

float featureMatch(float current, float target, float tolerance)
{
    if (!std::isfinite(current) || !std::isfinite(target))
    {
        return 0.0f;
    }
    return clamp01(1.0f - (std::abs(current - target) / std::max(tolerance, 0.001f)));
}
} // namespace

SegmentScores ScoringEngine::score(const FeatureFrame& feature, const SpectralFrame& spectral,
                                   const ReferenceSignature& referenceMatch,
                                   const ReferenceSignature& genreMean, float harshness, float mud,
                                   float air, float transientMatch, float stereoMatch,
                                   float subMonoIntegrity, float phaseRisk, float compressionRisk,
                                   float playbackSeconds) const
{
    SegmentScores out;
    const auto& targetFeature = referenceMatch.feature;
    const auto& fallbackFeature = genreMean.feature;
    const bool matchedFeatureTargetsUsable = signatureHasUsableFeatureTargets(referenceMatch);
    const bool fallbackFeatureTargetsUsable = signatureHasUsableFeatureTargets(genreMean);
    const float targetCrest = selectFeatureTarget(targetFeature.crestDb, matchedFeatureTargetsUsable,
                                                  fallbackFeature.crestDb,
                                                  fallbackFeatureTargetsUsable, 8.0f);
    const float targetTransient = selectFeatureTarget(targetFeature.transientDensity,
                                                      matchedFeatureTargetsUsable,
                                                      fallbackFeature.transientDensity,
                                                      fallbackFeatureTargetsUsable, 0.45f);
    const float targetSide = selectFeatureTarget(targetFeature.sideEnergy, matchedFeatureTargetsUsable,
                                                 fallbackFeature.sideEnergy,
                                                 fallbackFeatureTargetsUsable, 0.25f);
    const float targetCorrelation =
        selectFeatureTarget(targetFeature.correlation, matchedFeatureTargetsUsable,
                            fallbackFeature.correlation, fallbackFeatureTargetsUsable, 0.4f);
    const float targetMud = selectFeatureTarget(targetFeature.mud, matchedFeatureTargetsUsable,
                                                fallbackFeature.mud, fallbackFeatureTargetsUsable,
                                                0.35f);
    const float targetHarshness =
        selectFeatureTarget(targetFeature.harshness, matchedFeatureTargetsUsable,
                            fallbackFeature.harshness, fallbackFeatureTargetsUsable, 0.35f);
    const float targetAir = selectFeatureTarget(targetFeature.air, matchedFeatureTargetsUsable,
                                                fallbackFeature.air, fallbackFeatureTargetsUsable,
                                                0.45f);

    const float crestMatch = featureMatch(feature.crestDb, targetCrest, 4.5f);
    const float transientMeanMatch = featureMatch(feature.transientDensity, targetTransient, 0.18f);
    const float widthMatch = featureMatch(feature.sideEnergy, targetSide, 0.16f);
    const float correlationMatch = featureMatch(feature.correlation, targetCorrelation, 0.28f);
    const float harshnessMatch = featureMatch(harshness, targetHarshness, 0.18f);
    const float mudMatch = featureMatch(mud, targetMud, 0.18f);
    const float airMatch = featureMatch(air, targetAir, 0.18f);

    const float subTarget = hasUsableSpectralValue(referenceMatch.subBandDb) ? referenceMatch.subBandDb
                                                                              : genreMean.subBandDb;
    const float lowTarget = hasUsableSpectralValue(referenceMatch.lowBandDb) ? referenceMatch.lowBandDb
                                                                              : genreMean.lowBandDb;
    const float lowMidTarget =
        hasUsableSpectralValue(referenceMatch.lowMidBandDb) ? referenceMatch.lowMidBandDb
                                                            : genreMean.lowMidBandDb;
    const float midTarget = hasUsableSpectralValue(referenceMatch.midBandDb) ? referenceMatch.midBandDb
                                                                              : genreMean.midBandDb;
    const float highMidTarget =
        hasUsableSpectralValue(referenceMatch.highMidBandDb) ? referenceMatch.highMidBandDb
                                                             : genreMean.highMidBandDb;
    const float highTarget = hasUsableSpectralValue(referenceMatch.highBandDb) ? referenceMatch.highBandDb
                                                                                : genreMean.highBandDb;
    const float tiltTarget =
        hasUsableSpectralValue(referenceMatch.spectralTilt) ? referenceMatch.spectralTilt
                                                            : genreMean.spectralTilt;

    const float subMatch =
        usableBandMatch(spectral.subBandDb, subTarget, genreMean.subBandDb, 4.0f);
    const float lowMatch =
        usableBandMatch(spectral.lowBandDb, lowTarget, genreMean.lowBandDb, 3.5f);
    const float lowMidMatch =
        usableBandMatch(spectral.lowMidBandDb, lowMidTarget, genreMean.lowMidBandDb, 3.0f);
    const float midBandMatch =
        usableBandMatch(spectral.midBandDb, midTarget, genreMean.midBandDb, 3.0f);
    const float highMidMatch =
        usableBandMatch(spectral.highMidBandDb, highMidTarget, genreMean.highMidBandDb, 3.0f);
    const float highMatch =
        usableBandMatch(spectral.highBandDb, highTarget, genreMean.highBandDb, 3.5f);
    const float tiltMatch = spectralWindowMatch(spectral.spectralTiltDb, tiltTarget,
                                                genreMean.spectralTilt, 2.5f);
    const float vectorMatch = spectralVectorMatch(spectral, referenceMatch, genreMean);
    const float worstToneMatch =
        std::min({subMatch, lowMatch, lowMidMatch, midBandMatch, highMidMatch, highMatch});

    const float toneCore =
        clamp01(0.13f * subMatch + 0.17f * lowMatch + 0.20f * lowMidMatch + 0.16f * midBandMatch +
                0.15f * highMidMatch + 0.11f * highMatch + 0.08f * tiltMatch);
    out.tone = clamp01(0.55f * toneCore + 0.20f * vectorMatch + 0.10f * worstToneMatch +
                       0.05f * harshnessMatch + 0.05f * mudMatch + 0.05f * airMatch);
    out.dynamics = clamp01(0.42f * crestMatch + 0.28f * transientMeanMatch +
                           0.18f * transientMatch + 0.12f * (1.0f - compressionRisk));
    out.space = clamp01(0.36f * widthMatch + 0.24f * correlationMatch + 0.18f * stereoMatch +
                        0.12f * subMonoIntegrity + 0.10f * (1.0f - phaseRisk));
    out.punch = clamp01(0.38f * transientMatch + 0.30f * crestMatch +
                        0.20f * transientMeanMatch + 0.12f * (1.0f - compressionRisk));
    out.balance = clamp01(0.48f * out.tone + 0.24f * out.space + 0.14f * subMatch +
                          0.14f * lowMatch);

    out.behaviorIndex =
        clamp01(0.30f * out.punch + 0.24f * out.dynamics + 0.18f * out.space + 0.28f * out.tone);
    out.signalClarity =
        clamp01(0.30f * out.tone + 0.20f * out.dynamics + 0.15f * out.space +
                0.15f * vectorMatch + 0.10f * (1.0f - compressionRisk) +
                0.10f * (1.0f - phaseRisk));
    out.mixAlignment = clamp01(0.32f * out.tone + 0.18f * out.dynamics + 0.16f * out.space +
                               0.14f * out.punch + 0.20f * out.balance);

    // Legacy aliases used by older report/UI paths.
    out.life = out.behaviorIndex;
    out.signalStability = clamp01(0.55f * out.signalClarity + 0.45f * out.mixAlignment);
    out.confidence = out.signalStability;
    out.approval = out.mixAlignment;
    (void)playbackSeconds;

    return out;
}

} // namespace aifred::dsp
