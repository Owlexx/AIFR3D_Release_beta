#pragma once

#include <JuceHeader.h>

#include "../Common/AnalysisTypes.h"
#include "../Common/MixMetrics.h"
#include "../Interpretation/AifredReport.h"

#include <array>
#include <filesystem>
#include <vector>

struct Candle
{
    float open = 0;
    float close = 0;
    float high = 0;
    float low = 0;
    uint32_t flags = 0;
    double t0 = 0;
    double t1 = 0;
    float devLufs = 0.0f;
    float devTruePeak = 0.0f;
    float devCrest = 0.0f;
    float devSpectral = 0.0f;
    float devWidth = 0.0f;
    float devCorrelation = 0.0f;
    float devTransient = 0.0f;
};

struct FixCard
{
    juce::String id;
    juce::String title;
    juce::String why;
    juce::String next;
    juce::String impact;
    float certainty = 0.0f;
};

struct UiModel
{
    static constexpr std::size_t kMinuteCandleCount = 10;
    static constexpr std::size_t kSessionCandleCount = 10;

    juce::SmoothedValue<float> behaviorIndex{0.0f};
    juce::SmoothedValue<float> mixAlignment{0.0f};
    juce::SmoothedValue<float> signalClarity{0.0f};
    juce::SmoothedValue<float> signalStability{0.0f};
    juce::SmoothedValue<float> userMixSignature{0.0f};
    juce::SmoothedValue<float> refMixSignature{0.5f};

    juce::SmoothedValue<float> tone{0.0f}, dynamics{0.0f}, space{0.0f}, punch{0.0f}, balance{0.0f};

    juce::SmoothedValue<float> midEnergy{0.0f}, sideEnergy{0.0f};
    juce::SmoothedValue<float> correlation01{0.0f};
    juce::SmoothedValue<float> subMono{0.0f};
    juce::SmoothedValue<float> phaseRisk{0.0f};

    std::array<Candle, kSessionCandleCount> candleBuf{};
    int candleWrite = 0;
    std::array<Candle, kMinuteCandleCount> minuteCandleBuf{};
    int minuteCandleWrite = 0;

    std::vector<FixCard> fixList;
    std::vector<FixCard> diagnosticFixList;
    juce::String statusText = "Listening...";
    juce::String detectedGenre = "Unknown";
    float detectedGenreStability = 0.0f;
    float visualPhase = 0.0f;
    float visualPulse = 0.0f;

    uint32_t lastFlags = 0;

    // Persistent candlestick memory rooted from first observed session baseline.
    float firstSessionMixAverage = -1.0f;
    float lastReferenceDelta = 0.0f;
    float truePeakDbTP = -99.0f;
    float samplePeakDbFS = -99.0f;
    float rmsDbFS = -99.0f;
    float referenceTruePeakDbTP = -99.0f;
    float truePeakVsReferenceDb = 0.0f;
    float momentaryLUFS = -99.0f;
    float integratedLUFS = -99.0f;
    float shortTermLUFS = -99.0f;
    float crestFactorDb = 0.0f;
    float stereoWidth = 0.0f;
    float correlation = 0.0f;
    float spectralBalance = 0.0f;
    float transientDensity = 0.0f;
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
    float subBandDeltaDb = 0.0f;
    float lowBandDeltaDb = 0.0f;
    float lowMidBandDeltaDb = 0.0f;
    float midBandDeltaDb = 0.0f;
    float highMidBandDeltaDb = 0.0f;
    float highBandDeltaDb = 0.0f;
    float presenceBandDeltaDb = 0.0f;
    float airBandDeltaDb = 0.0f;
    juce::String haloLabel = "Listening";
    juce::String haloDetail = "Waiting for live buffer";
    juce::String tonalSummary = "No tonal read yet.";
    MixMetrics lastMetrics;
    AifredReport lastReport;

    void prepare(double uiHz);
    void updateFrom(const AnalysisFrame& f);
    void updateFrom(const MixMetrics& metrics, const AifredReport& report);
    void pushCandleFrom(const MixMetrics& metrics);
    void pushMinuteCandleFrom(const MixMetrics& metrics);
    void advance();

  private:
    void loadPersistentCandles();
    void persistCandlesIfNeeded();

    std::filesystem::path m_candleStatePath{"analysis/session_memory/vst_session_store_v1.json"};
    bool m_memoryLoaded = false;
    bool m_memoryDirty = false;
    int m_persistTick = 0;

    bool m_sessionActive = false;
    double m_sessionStartSec = 0.0;
    double m_lastActiveSec = 0.0;
    float m_sessionOpen = 0.5f;
    float m_sessionClose = 0.5f;
    float m_sessionHigh = 0.5f;
    float m_sessionLow = 0.5f;
    uint32_t m_sessionFlags = 0;
    int m_sessionSamples = 0;
    float m_sessionDeltaSum = 0.0f;
    float m_sessionLufsSum = 0.0f;
    float m_sessionTruePeakSum = 0.0f;
    float m_sessionRmsSum = 0.0f;
    float m_sessionCrestSum = 0.0f;
    float m_sessionSpectralSum = 0.0f;
    float m_sessionWidthSum = 0.0f;
    float m_sessionCorrSum = 0.0f;
    float m_sessionTransientSum = 0.0f;
    int m_fixHoldFrames = 0;

    bool m_hasPreviousSessionMetrics = false;
    float m_prevSessionLufs = 0.0f;
    float m_prevSessionTruePeak = 0.0f;
    float m_prevSessionRms = 0.0f;
    float m_prevSessionCrest = 0.0f;
    float m_prevSessionSpectral = 0.0f;
    float m_prevSessionWidth = 0.0f;
    float m_prevSessionCorr = 0.0f;
    float m_prevSessionTransient = 0.0f;

    bool m_hasMinuteRollingAverage = false;
    float m_minuteAvgMomentary = -24.0f;
    float m_minuteAvgTruePeak = -6.0f;
    float m_minuteAvgWidth = 0.25f;
    float m_minuteAvgTilt = 0.0f;
    float m_minuteAvgTransient = 0.08f;
    bool m_minuteCandleActive = false;
    double m_minuteCandleStartSec = 0.0;
    float m_minuteCandleOpen = 0.0f;
    float m_minuteCandleClose = 0.0f;
    float m_minuteCandleHigh = 0.0f;
    float m_minuteCandleLow = 0.0f;
    int m_minuteCandleSamples = 0;
};
