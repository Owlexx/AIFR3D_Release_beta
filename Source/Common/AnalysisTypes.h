#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

enum AnalysisFlags : uint32_t
{
    kFlagNone = 0,
    kFlagTransientSoftening = 1u << 0,
    kFlagHarshnessBurst = 1u << 1,
    kFlagWidthCollapse = 1u << 2,
    kFlagClipRisk = 1u << 3,
    kFlagPhaseRisk = 1u << 4
};

enum FrequencyBalanceStatusCode : uint8_t
{
    kFrequencyBalanceStatusNone = 0,
    kFrequencyBalanceStatusSubExcessive = 1,
    kFrequencyBalanceStatusSubLacking = 2,
    kFrequencyBalanceStatusBassHeavy = 3,
    kFrequencyBalanceStatusBassThin = 4,
    kFrequencyBalanceStatusMuddy = 5,
    kFrequencyBalanceStatusLowMidHollow = 6,
    kFrequencyBalanceStatusMidHeavy = 7,
    kFrequencyBalanceStatusMidScooped = 8,
    kFrequencyBalanceStatusHarsh = 9,
    kFrequencyBalanceStatusDull = 10,
    kFrequencyBalanceStatusSibilant = 11,
    kFrequencyBalanceStatusDark = 12
};

enum IntegratedLoudnessStatusCode : uint8_t
{
    kIntegratedLoudnessStatusNone = 0,
    kIntegratedLoudnessStatusCriticallyLoud = 1,
    kIntegratedLoudnessStatusTooLoud = 2,
    kIntegratedLoudnessStatusSlightlyLoud = 3,
    kIntegratedLoudnessStatusOptimal = 4,
    kIntegratedLoudnessStatusTooQuiet = 5,
    kIntegratedLoudnessStatusCriticallyQuiet = 6
};

enum DynamicRangeStatusCode : uint8_t
{
    kDynamicRangeStatusNone = 0,
    kDynamicRangeStatusHighlyDynamic = 1,
    kDynamicRangeStatusDynamic = 2,
    kDynamicRangeStatusBalanced = 3,
    kDynamicRangeStatusCompressed = 4,
    kDynamicRangeStatusHeavilyCompressed = 5,
    kDynamicRangeStatusBrickWalled = 6
};

enum AnalysisStateCode : uint8_t
{
    kAnalysisStateIdle = 0,
    kAnalysisStateBufferWarming = 1,
    kAnalysisStateActiveLive = 2,
    kAnalysisStateValidScored = 3,
    kAnalysisStateInvalidReference = 4,
    kAnalysisStateReferenceBlocked = 5
};

enum ReferenceStateCode : uint8_t
{
    kReferenceStateUnknown = 0,
    kReferenceStateReady = 1,
    kReferenceStateIncomplete = 2,
    kReferenceStateBlocked = 3
};

struct MeasurementFrame
{
    float integratedLUFS = -99.0f;
    float shortTermLUFS = -99.0f;
    float momentaryLUFS = -99.0f;
    float truePeakDbTP = -99.0f;
    float peakDbFS = -99.0f;
    float crestFactorDb = 0.0f;
    float dynamicRangeDb = 0.0f;
    float stereoWidth = 0.0f;
    float correlation = 0.0f;
    float phaseRisk = 0.0f;
    float transientDensity = 0.0f;
    float transientRateHz = 0.0f;
    float spectralBalance = 0.0f;
    std::array<float, 8> spectralBandsDb{
        -90.0f, -90.0f, -90.0f, -90.0f, -90.0f, -90.0f, -90.0f, -90.0f};
};

struct DeviationFrame
{
    float loudnessDeltaLUFS = 0.0f;
    float truePeakDeltaDb = 0.0f;
    float dynamicRangeDeltaDb = 0.0f;
    float widthDelta = 0.0f;
    float correlationDelta = 0.0f;
    float spectralTiltDeltaDb = 0.0f;
    float targetWindowDb = 3.0f;
    std::array<float, 8> bandDeltaDb{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

struct BehaviorFrame
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

struct AnalysisFrame
{
    double tSec = 0.0;

    // Canonical user-facing metric names.
    float behaviorIndex = 0.0f;
    float mixAlignment = 0.0f;
    float signalClarity = 0.0f;

    // Deprecated aliases retained for compatibility.
    float lifeIndex = 0.0f;
    float approval = 0.0f;
    float signalStability = 0.0f;
    float confidence = 0.0f;

    float segTone = 0.0f;
    float segDyn = 0.0f;
    float segSpace = 0.0f;
    float segPunch = 0.0f;
    float segBalance = 0.0f;

    MeasurementFrame measurement;
    DeviationFrame deviation;
    BehaviorFrame behavior;

    float integratedLUFS = -99.0f;
    float shortTermLUFS = -99.0f;
    float momentaryLUFS = -99.0f;
    float truePeakDbTP = -99.0f;
    float peakDbFS = -99.0f;
    float integratedLoudnessReferenceLUFS = -14.0f;
    float referenceTruePeakDbTP = -99.0f;
    float targetTruePeakDbTP = -99.0f;
    float truePeakVsReferenceDb = 0.0f;
    uint8_t integratedLoudnessStatusCode = kIntegratedLoudnessStatusNone;
    uint8_t integratedLoudnessSeverity = 0;
    float crestFactorDb = 0.0f;
    float dynamicRangeDb = 0.0f;
    float dynamicRangeReferenceDb = 8.0f;
    uint8_t dynamicRangeStatusCode = kDynamicRangeStatusNone;
    uint8_t dynamicRangeSeverity = 0;
    float stereoWidth = 0.0f;
    float spectralBalance = 0.0f;
    float transientDensity = 0.0f;
    float transientRateHz = 0.0f;
    float volatilityIndex = 0.0f;

    float subBandDb = -90.0f;
    float lowBandDb = -90.0f;
    float lowMidBandDb = -90.0f;
    float midBandDb = -90.0f;
    float highMidBandDb = -90.0f;
    float highBandDb = -90.0f;
    float presenceBandDb = -90.0f;
    float airBandDb = -90.0f;

    float referenceSubBandDb = -90.0f;
    float referenceLowBandDb = -90.0f;
    float referenceLowMidBandDb = -90.0f;
    float referenceMidBandDb = -90.0f;
    float referenceHighMidBandDb = -90.0f;
    float referenceHighBandDb = -90.0f;
    float referencePresenceBandDb = -90.0f;
    float referenceAirBandDb = -90.0f;
    float targetSubBandDb = -90.0f;
    float targetLowBandDb = -90.0f;
    float targetLowMidBandDb = -90.0f;
    float targetMidBandDb = -90.0f;
    float targetHighMidBandDb = -90.0f;
    float targetHighBandDb = -90.0f;
    float targetPresenceBandDb = -90.0f;
    float targetAirBandDb = -90.0f;
    float targetWindowDb = 3.0f;

    float frequencySubEnergy = -90.0f;
    float frequencyBassEnergy = -90.0f;
    float frequencyLowMidEnergy = -90.0f;
    float frequencyMidEnergy = -90.0f;
    float frequencyHighMidEnergy = -90.0f;
    float frequencyPresenceEnergy = -90.0f;
    float frequencyAirEnergy = -90.0f;
    float targetFrequencySubEnergy = -90.0f;
    float targetFrequencyBassEnergy = -90.0f;
    float targetFrequencyLowMidEnergy = -90.0f;
    float targetFrequencyMidEnergy = -90.0f;
    float targetFrequencyHighMidEnergy = -90.0f;
    float targetFrequencyPresenceEnergy = -90.0f;
    float targetFrequencyAirEnergy = -90.0f;
    uint8_t frequencyBalanceStatusCode = kFrequencyBalanceStatusNone;
    uint8_t frequencyBalanceSeverity = 0;

    float subBandDeltaDb = 0.0f;
    float lowBandDeltaDb = 0.0f;
    float lowMidBandDeltaDb = 0.0f;
    float midBandDeltaDb = 0.0f;
    float highMidBandDeltaDb = 0.0f;
    float highBandDeltaDb = 0.0f;
    float presenceBandDeltaDb = 0.0f;
    float airBandDeltaDb = 0.0f;

    float midEnergy = 0.0f;
    float sideEnergy = 0.0f;
    float leftEnergy = 0.5f;
    float rightEnergy = 0.5f;
    float correlation = 0.0f;
    float subMonoIntegrity = 0.0f;
    float phaseRisk = 0.0f;
    float lowBandCorrelation = 1.0f;
    float lowBandPhaseRisk = 0.0f;
    float centerDominance = 0.5f;
    float sideDominance = 0.0f;
    float stereoMotion = 0.0f;
    float spatialSpread = 0.0f;
    float spatialW = 0.5f;
    float spatialX = 0.0f;
    float spatialY = 0.5f;
    float spatialZ = 0.0f;
    float referenceStereoWidth = 0.0f;
    float referenceCorrelation = 0.0f;
    float targetStereoWidth = 0.0f;
    float targetCorrelation = 0.0f;

    float dynOpen = 0.0f;
    float dynClose = 0.0f;
    float dynHigh = 0.0f;
    float dynLow = 0.0f;

    // 0 unknown, 1 pop, 2 edm, 3 hip-hop, 4 rap, 5 dubstep, 6 rock
    uint8_t genreIndex = 0;
    float genreConfidence = 0.0f;
    uint8_t analysisStateCode = kAnalysisStateIdle;
    uint8_t referenceStateCode = kReferenceStateUnknown;
    uint8_t referenceValid = 0;
    uint8_t scoredResultValid = 0;
    uint8_t liveSignalPresent = 0;

    uint32_t flags = kFlagNone;
};

inline float clamp01(float x)
{
    if (!std::isfinite(x))
    {
        return 0.0f;
    }
    return juce::jlimit(0.0f, 1.0f, x);
}
