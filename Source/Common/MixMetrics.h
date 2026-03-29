#pragma once

#include "AnalysisTypes.h"

struct LoudnessMetrics
{
    float integratedLufs = -99.0f;
    float shortTermLufs = -99.0f;
    float momentaryLufs = -99.0f;
    float truePeakDbTP = -99.0f;
    float integratedReferenceLufs = -14.0f;
    float referenceTruePeakDbTP = -99.0f;
    float targetTruePeakDbTP = -99.0f;
    float truePeakVsReferenceDb = 0.0f;
    std::uint8_t statusCode = kIntegratedLoudnessStatusNone;
    std::uint8_t severity = 0;
};

struct StereoMetrics
{
    float width = 0.0f;
    float correlation = 0.0f;
    float midEnergy = 0.0f;
    float sideEnergy = 0.0f;
    float leftEnergy = 0.5f;
    float rightEnergy = 0.5f;
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
    float referenceWidth = 0.0f;
    float referenceCorrelation = 0.0f;
    float targetWidth = 0.0f;
    float targetCorrelation = 0.0f;
};

struct DynamicsMetrics
{
    float peakDbFs = -99.0f;
    float crestFactorDb = 0.0f;
    float dynamicRangeDb = 0.0f;
    float dynamicRangeReferenceDb = 8.0f;
    std::uint8_t statusCode = kDynamicRangeStatusNone;
    std::uint8_t severity = 0;
    float transientDensity = 0.0f;
    float transientRateHz = 0.0f;
    float volatilityIndex = 0.0f;
    float dynOpen = 0.0f;
    float dynClose = 0.0f;
    float dynHigh = 0.0f;
    float dynLow = 0.0f;
};

struct TonalMetrics
{
    float spectralBalance = 0.0f;
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
};

struct ReferenceDeltas
{
    float subBandDb = 0.0f;
    float lowBandDb = 0.0f;
    float lowMidBandDb = 0.0f;
    float midBandDb = 0.0f;
    float highMidBandDb = 0.0f;
    float highBandDb = 0.0f;
    float presenceBandDb = 0.0f;
    float airBandDb = 0.0f;
};

struct FrequencyBalanceMetrics
{
    float subEnergy = -90.0f;
    float bassEnergy = -90.0f;
    float lowMidEnergy = -90.0f;
    float midEnergy = -90.0f;
    float highMidEnergy = -90.0f;
    float presenceEnergy = -90.0f;
    float airEnergy = -90.0f;
    float targetSub = -90.0f;
    float targetBass = -90.0f;
    float targetLowMid = -90.0f;
    float targetMid = -90.0f;
    float targetHighMid = -90.0f;
    float targetPresence = -90.0f;
    float targetAir = -90.0f;
    std::uint8_t statusCode = kFrequencyBalanceStatusNone;
    std::uint8_t severity = 0;
};

struct ScoringMetrics
{
    float behaviorIndex = 0.0f;
    float mixAlignment = 0.0f;
    float signalClarity = 0.0f;
    float signalStability = 0.0f;
    float tone = 0.0f;
    float dynamics = 0.0f;
    float space = 0.0f;
    float punch = 0.0f;
    float balance = 0.0f;
};

struct MixMetrics
{
    double tSec = 0.0;
    LoudnessMetrics loudness;
    StereoMetrics stereo;
    DynamicsMetrics dynamics;
    TonalMetrics tonal;
    ReferenceDeltas referenceDeltas;
    FrequencyBalanceMetrics frequencyBalance;
    ScoringMetrics scoring;
    std::uint8_t genreIndex = 0;
    float genreConfidence = 0.0f;
    std::uint8_t analysisStateCode = kAnalysisStateIdle;
    std::uint8_t referenceStateCode = kReferenceStateUnknown;
    bool referenceValid = false;
    bool scoredResultValid = false;
    bool liveSignalPresent = false;
    std::uint32_t flags = kFlagNone;

    static MixMetrics fromAnalysisFrame(const AnalysisFrame& frame)
    {
        MixMetrics metrics;
        metrics.tSec = frame.tSec;
        metrics.loudness.integratedLufs = frame.integratedLUFS;
        metrics.loudness.shortTermLufs = frame.shortTermLUFS;
        metrics.loudness.momentaryLufs = frame.momentaryLUFS;
        metrics.loudness.truePeakDbTP = frame.truePeakDbTP;
        metrics.loudness.integratedReferenceLufs = frame.integratedLoudnessReferenceLUFS;
        metrics.loudness.referenceTruePeakDbTP = frame.referenceTruePeakDbTP;
        metrics.loudness.targetTruePeakDbTP = frame.targetTruePeakDbTP;
        metrics.loudness.truePeakVsReferenceDb = frame.truePeakVsReferenceDb;
        metrics.loudness.statusCode = frame.integratedLoudnessStatusCode;
        metrics.loudness.severity = frame.integratedLoudnessSeverity;

        metrics.stereo.width = frame.stereoWidth;
        metrics.stereo.correlation = frame.correlation;
        metrics.stereo.midEnergy = frame.midEnergy;
        metrics.stereo.sideEnergy = frame.sideEnergy;
        metrics.stereo.leftEnergy = frame.leftEnergy;
        metrics.stereo.rightEnergy = frame.rightEnergy;
        metrics.stereo.subMonoIntegrity = frame.subMonoIntegrity;
        metrics.stereo.phaseRisk = frame.phaseRisk;
        metrics.stereo.lowBandCorrelation = frame.lowBandCorrelation;
        metrics.stereo.lowBandPhaseRisk = frame.lowBandPhaseRisk;
        metrics.stereo.centerDominance = frame.centerDominance;
        metrics.stereo.sideDominance = frame.sideDominance;
        metrics.stereo.stereoMotion = frame.stereoMotion;
        metrics.stereo.spatialSpread = frame.spatialSpread;
        metrics.stereo.spatialW = frame.spatialW;
        metrics.stereo.spatialX = frame.spatialX;
        metrics.stereo.spatialY = frame.spatialY;
        metrics.stereo.spatialZ = frame.spatialZ;
        metrics.stereo.referenceWidth = frame.referenceStereoWidth;
        metrics.stereo.referenceCorrelation = frame.referenceCorrelation;
        metrics.stereo.targetWidth = frame.targetStereoWidth;
        metrics.stereo.targetCorrelation = frame.targetCorrelation;

        metrics.dynamics.peakDbFs = frame.peakDbFS;
        metrics.dynamics.crestFactorDb = frame.crestFactorDb;
        metrics.dynamics.dynamicRangeDb = frame.dynamicRangeDb;
        metrics.dynamics.dynamicRangeReferenceDb = frame.dynamicRangeReferenceDb;
        metrics.dynamics.statusCode = frame.dynamicRangeStatusCode;
        metrics.dynamics.severity = frame.dynamicRangeSeverity;
        metrics.dynamics.transientDensity = frame.transientDensity;
        metrics.dynamics.transientRateHz = frame.transientRateHz;
        metrics.dynamics.volatilityIndex = frame.volatilityIndex;
        metrics.dynamics.dynOpen = frame.dynOpen;
        metrics.dynamics.dynClose = frame.dynClose;
        metrics.dynamics.dynHigh = frame.dynHigh;
        metrics.dynamics.dynLow = frame.dynLow;

        metrics.tonal.spectralBalance = frame.spectralBalance;
        metrics.tonal.subBandDb = frame.subBandDb;
        metrics.tonal.lowBandDb = frame.lowBandDb;
        metrics.tonal.lowMidBandDb = frame.lowMidBandDb;
        metrics.tonal.midBandDb = frame.midBandDb;
        metrics.tonal.highMidBandDb = frame.highMidBandDb;
        metrics.tonal.highBandDb = frame.highBandDb;
        metrics.tonal.presenceBandDb = frame.presenceBandDb;
        metrics.tonal.airBandDb = frame.airBandDb;
        metrics.tonal.referenceSubBandDb = frame.referenceSubBandDb;
        metrics.tonal.referenceLowBandDb = frame.referenceLowBandDb;
        metrics.tonal.referenceLowMidBandDb = frame.referenceLowMidBandDb;
        metrics.tonal.referenceMidBandDb = frame.referenceMidBandDb;
        metrics.tonal.referenceHighMidBandDb = frame.referenceHighMidBandDb;
        metrics.tonal.referenceHighBandDb = frame.referenceHighBandDb;
        metrics.tonal.referencePresenceBandDb = frame.referencePresenceBandDb;
        metrics.tonal.referenceAirBandDb = frame.referenceAirBandDb;

        metrics.referenceDeltas.subBandDb = frame.subBandDeltaDb;
        metrics.referenceDeltas.lowBandDb = frame.lowBandDeltaDb;
        metrics.referenceDeltas.lowMidBandDb = frame.lowMidBandDeltaDb;
        metrics.referenceDeltas.midBandDb = frame.midBandDeltaDb;
        metrics.referenceDeltas.highMidBandDb = frame.highMidBandDeltaDb;
        metrics.referenceDeltas.highBandDb = frame.highBandDeltaDb;
        metrics.referenceDeltas.presenceBandDb = frame.presenceBandDeltaDb;
        metrics.referenceDeltas.airBandDb = frame.airBandDeltaDb;

        metrics.frequencyBalance.subEnergy = frame.frequencySubEnergy;
        metrics.frequencyBalance.bassEnergy = frame.frequencyBassEnergy;
        metrics.frequencyBalance.lowMidEnergy = frame.frequencyLowMidEnergy;
        metrics.frequencyBalance.midEnergy = frame.frequencyMidEnergy;
        metrics.frequencyBalance.highMidEnergy = frame.frequencyHighMidEnergy;
        metrics.frequencyBalance.presenceEnergy = frame.frequencyPresenceEnergy;
        metrics.frequencyBalance.airEnergy = frame.frequencyAirEnergy;
        metrics.frequencyBalance.targetSub = frame.targetFrequencySubEnergy;
        metrics.frequencyBalance.targetBass = frame.targetFrequencyBassEnergy;
        metrics.frequencyBalance.targetLowMid = frame.targetFrequencyLowMidEnergy;
        metrics.frequencyBalance.targetMid = frame.targetFrequencyMidEnergy;
        metrics.frequencyBalance.targetHighMid = frame.targetFrequencyHighMidEnergy;
        metrics.frequencyBalance.targetPresence = frame.targetFrequencyPresenceEnergy;
        metrics.frequencyBalance.targetAir = frame.targetFrequencyAirEnergy;
        metrics.frequencyBalance.statusCode = frame.frequencyBalanceStatusCode;
        metrics.frequencyBalance.severity = frame.frequencyBalanceSeverity;

        metrics.scoring.behaviorIndex = frame.behaviorIndex;
        metrics.scoring.mixAlignment = frame.mixAlignment;
        metrics.scoring.signalClarity = frame.signalClarity;
        metrics.scoring.signalStability = frame.signalStability;
        metrics.scoring.tone = frame.segTone;
        metrics.scoring.dynamics = frame.segDyn;
        metrics.scoring.space = frame.segSpace;
        metrics.scoring.punch = frame.segPunch;
        metrics.scoring.balance = frame.segBalance;

        metrics.genreIndex = frame.genreIndex;
        metrics.genreConfidence = frame.genreConfidence;
        metrics.analysisStateCode = frame.analysisStateCode;
        metrics.referenceStateCode = frame.referenceStateCode;
        metrics.referenceValid = frame.referenceValid != 0;
        metrics.scoredResultValid = frame.scoredResultValid != 0;
        metrics.liveSignalPresent = frame.liveSignalPresent != 0;
        metrics.flags = frame.flags;
        return metrics;
    }
};
