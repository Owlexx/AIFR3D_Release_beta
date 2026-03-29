#include "UiModel.h"

#include "../Interpretation/DiagnosticEngine.h"
#include "coresynth/audiosuite_core/session_store.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>

namespace
{
void setSmooth(juce::SmoothedValue<float>& v, double srHz, double attackMs, double releaseMs)
{
    const double rampSec = juce::jlimit(0.02, 0.10, (attackMs + releaseMs) / 6000.0);
    v.reset(srHz, rampSec);
    v.setCurrentAndTargetValue(v.getCurrentValue());
}

juce::String genreNameFromIndex(uint8_t index)
{
    switch (index)
    {
    case 1:
        return "Pop";
    case 2:
        return "EDM";
    case 3:
        return "Hip-Hop";
    case 4:
        return "Rap";
    case 5:
        return "Dubstep";
    case 6:
        return "Rock";
    default:
        return "Unknown";
    }
}

float clampPrice(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float normalisedDistance(float delta, float scale)
{
    return delta / juce::jmax(0.001f, scale);
}

float deviationScore(const MixMetrics& metrics, float avgMomentary, float avgTruePeak,
                     float avgWidth, float avgTilt, float avgTransient)
{
    const float dMomentary =
        normalisedDistance(metrics.loudness.momentaryLufs - avgMomentary, 6.0f);
    const float dTruePeak =
        normalisedDistance(metrics.loudness.truePeakDbTP - avgTruePeak, 3.0f);
    const float dWidth = normalisedDistance(metrics.stereo.width - avgWidth, 0.22f);
    const float dTilt = normalisedDistance(metrics.tonal.spectralBalance - avgTilt, 6.0f);
    const float dTransient =
        normalisedDistance(metrics.dynamics.transientDensity - avgTransient, 0.12f);

    const float weighted =
        (0.28f * dMomentary * dMomentary) + (0.22f * dTruePeak * dTruePeak) +
        (0.18f * dWidth * dWidth) + (0.16f * dTilt * dTilt) +
        (0.16f * dTransient * dTransient);
    return std::sqrt(std::max(0.0f, weighted));
}

} // namespace

void UiModel::loadPersistentCandles()
{
    m_memoryLoaded = true;
    coresynth::audiosuite_core::SessionStore store(m_candleStatePath, kSessionCandleCount);
    const auto state = store.load();
    firstSessionMixAverage = state.firstSessionBaseline;
    candleWrite = static_cast<int>(state.writeIndex % kSessionCandleCount);

    for (std::size_t i = 0; i < candleBuf.size() && i < state.candles.size(); ++i)
    {
        candleBuf[i].open = clampPrice(state.candles[i].open);
        candleBuf[i].close = clampPrice(state.candles[i].close);
        candleBuf[i].high = clampPrice(state.candles[i].high);
        candleBuf[i].low = clampPrice(state.candles[i].low);
        candleBuf[i].flags = state.candles[i].flags;
        candleBuf[i].t0 = state.candles[i].t0;
        candleBuf[i].t1 = state.candles[i].t1;
        candleBuf[i].devLufs = static_cast<float>(state.candles[i].deviation.integratedLufs);
        candleBuf[i].devTruePeak = static_cast<float>(state.candles[i].deviation.truePeakDbtp);
        candleBuf[i].devCrest = static_cast<float>(state.candles[i].deviation.crestFactorDb);
        candleBuf[i].devSpectral = static_cast<float>(state.candles[i].deviation.spectralBalance);
        candleBuf[i].devWidth = static_cast<float>(state.candles[i].deviation.stereoWidth);
        candleBuf[i].devCorrelation =
            static_cast<float>(state.candles[i].deviation.stereoCorrelation);
        candleBuf[i].devTransient = static_cast<float>(state.candles[i].deviation.transientDensity);
    }
}

void UiModel::persistCandlesIfNeeded()
{
    if (!m_memoryDirty)
    {
        return;
    }

    if (++m_persistTick < 24)
    {
        return;
    }
    m_persistTick = 0;

    coresynth::audiosuite_core::SessionStore store(m_candleStatePath, kSessionCandleCount);
    coresynth::audiosuite_core::SessionStoreData state;
    state.firstSessionBaseline = firstSessionMixAverage;
    state.maxSessions = kSessionCandleCount;
    state.writeIndex = static_cast<std::size_t>(std::max(0, candleWrite));
    state.candles.reserve(candleBuf.size());

    for (const auto& candle : candleBuf)
    {
        coresynth::audiosuite_core::SessionCandleRecord record;
        record.open = candle.open;
        record.close = candle.close;
        record.high = candle.high;
        record.low = candle.low;
        record.flags = candle.flags;
        record.t0 = candle.t0;
        record.t1 = candle.t1;
        record.deviation.integratedLufs = candle.devLufs;
        record.deviation.truePeakDbtp = candle.devTruePeak;
        record.deviation.crestFactorDb = candle.devCrest;
        record.deviation.spectralBalance = candle.devSpectral;
        record.deviation.stereoWidth = candle.devWidth;
        record.deviation.stereoCorrelation = candle.devCorrelation;
        record.deviation.transientDensity = candle.devTransient;
        state.candles.push_back(record);
    }

    (void)store.save(std::move(state));
    store.appendLogLine("vst session candle store persisted");
    m_memoryDirty = false;
}

void UiModel::prepare(double uiHz)
{
    const double sr = uiHz;

    setSmooth(behaviorIndex, sr, 50, 250);
    setSmooth(mixAlignment, sr, 250, 600);
    setSmooth(signalClarity, sr, 200, 500);
    setSmooth(signalStability, sr, 200, 500);
    userMixSignature.reset(sr, 8.0);
    refMixSignature.reset(sr, 12.0);
    userMixSignature.setCurrentAndTargetValue(userMixSignature.getCurrentValue());
    refMixSignature.setCurrentAndTargetValue(refMixSignature.getCurrentValue());

    setSmooth(tone, sr, 250, 600);
    setSmooth(dynamics, sr, 250, 600);
    setSmooth(space, sr, 250, 600);
    setSmooth(punch, sr, 250, 600);
    setSmooth(balance, sr, 250, 600);

    setSmooth(midEnergy, sr, 120, 250);
    setSmooth(sideEnergy, sr, 120, 250);
    setSmooth(correlation01, sr, 150, 300);
    setSmooth(subMono, sr, 150, 300);
    setSmooth(phaseRisk, sr, 80, 150);

    if (!m_memoryLoaded)
    {
        loadPersistentCandles();
    }
}

void UiModel::pushCandleFrom(const MixMetrics& metrics)
{
    constexpr double kSessionSilenceTimeoutSec = 2.5;
    constexpr double kMinSessionDurationSec = 20.0;
    constexpr float kActivityLufsThreshold = -62.0f;
    constexpr float kActivityTransientThreshold = 0.015f;

    const float signatureDelta = juce::jlimit(
        -1.0f, 1.0f, userMixSignature.getTargetValue() - refMixSignature.getTargetValue());
    const float signaturePrice = clampPrice(0.5f + signatureDelta * 0.28f);
    const bool isActiveFrame = (metrics.loudness.shortTermLufs > kActivityLufsThreshold) ||
                               (metrics.dynamics.transientDensity > kActivityTransientThreshold);

    if (!m_sessionActive && isActiveFrame)
    {
        const float baseline = (firstSessionMixAverage < 0.0f) ? 0.5f : firstSessionMixAverage;
        const int prevIndex = (candleWrite == 0) ? static_cast<int>(candleBuf.size() - 1)
                                                 : (candleWrite - 1);
        const float prevClose = candleBuf[static_cast<std::size_t>(prevIndex)].close;

        m_sessionOpen = clampPrice(prevClose <= 0.001f ? baseline : prevClose);
        m_sessionClose = m_sessionOpen;
        m_sessionHigh = m_sessionOpen;
        m_sessionLow = m_sessionOpen;
        m_sessionFlags = 0;
        m_sessionStartSec = metrics.tSec;
        m_lastActiveSec = metrics.tSec;
        m_sessionSamples = 0;
        m_sessionDeltaSum = 0.0f;
        m_sessionLufsSum = 0.0f;
        m_sessionTruePeakSum = 0.0f;
        m_sessionRmsSum = 0.0f;
        m_sessionCrestSum = 0.0f;
        m_sessionSpectralSum = 0.0f;
        m_sessionWidthSum = 0.0f;
        m_sessionCorrSum = 0.0f;
        m_sessionTransientSum = 0.0f;
        m_sessionActive = true;
    }

    if (!m_sessionActive)
    {
        return;
    }

    if (isActiveFrame)
    {
        m_lastActiveSec = metrics.tSec;
        m_sessionClose = signaturePrice;
        m_sessionHigh = juce::jmax(m_sessionHigh, signaturePrice);
        m_sessionLow = juce::jmin(m_sessionLow, signaturePrice);
        m_sessionFlags |= metrics.flags;
        ++m_sessionSamples;

        m_sessionDeltaSum += signatureDelta;
        m_sessionLufsSum += metrics.loudness.integratedLufs;
        m_sessionTruePeakSum += metrics.loudness.truePeakDbTP;
        m_sessionRmsSum += metrics.loudness.rmsDbFS;
        m_sessionCrestSum += metrics.dynamics.crestFactorDb;
        m_sessionSpectralSum += metrics.tonal.spectralBalance;
        m_sessionWidthSum += metrics.stereo.width;
        m_sessionCorrSum += metrics.stereo.correlation;
        m_sessionTransientSum += metrics.dynamics.transientDensity;
    }

    const double sessionDurationSec = metrics.tSec - m_sessionStartSec;
    const bool sessionTimedOut =
        !isActiveFrame && ((metrics.tSec - m_lastActiveSec) >= kSessionSilenceTimeoutSec);
    if (!sessionTimedOut)
    {
        return;
    }

    if (sessionDurationSec < kMinSessionDurationSec || m_sessionSamples < 8)
    {
        m_sessionActive = false;
        return;
    }

    Candle committed;
    committed.open = m_sessionOpen;
    committed.close = m_sessionClose;
    committed.high = m_sessionHigh;
    committed.low = m_sessionLow;
    committed.flags = m_sessionFlags;
    committed.t0 = m_sessionStartSec;
    committed.t1 = m_lastActiveSec;

    const int samples = juce::jmax(1, m_sessionSamples);
    const float inv = 1.0f / static_cast<float>(samples);
    const float avgLufs = m_sessionLufsSum * inv;
    const float avgTruePeak = m_sessionTruePeakSum * inv;
    const float avgRms = m_sessionRmsSum * inv;
    const float avgCrest = m_sessionCrestSum * inv;
    const float avgSpectral = m_sessionSpectralSum * inv;
    const float avgWidth = m_sessionWidthSum * inv;
    const float avgCorr = m_sessionCorrSum * inv;
    const float avgTransient = m_sessionTransientSum * inv;

    if (firstSessionMixAverage < 0.0f)
    {
        firstSessionMixAverage = committed.close;
    }

    if (m_hasPreviousSessionMetrics)
    {
        committed.devLufs = avgLufs - m_prevSessionLufs;
        committed.devTruePeak = avgTruePeak - m_prevSessionTruePeak;
        committed.devCrest = avgCrest - m_prevSessionCrest;
        committed.devSpectral = avgSpectral - m_prevSessionSpectral;
        committed.devWidth = avgWidth - m_prevSessionWidth;
        committed.devCorrelation = avgCorr - m_prevSessionCorr;
        committed.devTransient = avgTransient - m_prevSessionTransient;
    }

    m_prevSessionLufs = avgLufs;
    m_prevSessionTruePeak = avgTruePeak;
    m_prevSessionRms = avgRms;
    m_prevSessionCrest = avgCrest;
    m_prevSessionSpectral = avgSpectral;
    m_prevSessionWidth = avgWidth;
    m_prevSessionCorr = avgCorr;
    m_prevSessionTransient = avgTransient;
    m_hasPreviousSessionMetrics = true;

    lastReferenceDelta = m_sessionDeltaSum * inv;

    candleBuf[static_cast<std::size_t>(candleWrite)] = committed;
    candleWrite = (candleWrite + 1) % static_cast<int>(candleBuf.size());
    m_memoryDirty = true;
    m_sessionActive = false;
}

void UiModel::pushMinuteCandleFrom(const MixMetrics& metrics)
{
    constexpr double kMinuteCandleDurationSec = 6.0;
    constexpr float kAverageAlpha = 0.06f;

    if (!m_hasMinuteRollingAverage)
    {
        m_minuteAvgMomentary = metrics.loudness.momentaryLufs;
        m_minuteAvgTruePeak = metrics.loudness.truePeakDbTP;
        m_minuteAvgWidth = metrics.stereo.width;
        m_minuteAvgTilt = metrics.tonal.spectralBalance;
        m_minuteAvgTransient = metrics.dynamics.transientDensity;
        m_hasMinuteRollingAverage = true;
    }

    const float deviation =
        clampPrice(deviationScore(metrics, m_minuteAvgMomentary, m_minuteAvgTruePeak,
                                  m_minuteAvgWidth, m_minuteAvgTilt, m_minuteAvgTransient) /
                   1.6f);

    if (!m_minuteCandleActive)
    {
        m_minuteCandleActive = true;
        m_minuteCandleStartSec = metrics.tSec;
        m_minuteCandleOpen = deviation;
        m_minuteCandleClose = deviation;
        m_minuteCandleHigh = deviation;
        m_minuteCandleLow = deviation;
        m_minuteCandleSamples = 1;
    }
    else
    {
        m_minuteCandleClose = deviation;
        m_minuteCandleHigh = juce::jmax(m_minuteCandleHigh, deviation);
        m_minuteCandleLow = juce::jmin(m_minuteCandleLow, deviation);
        ++m_minuteCandleSamples;
    }

    if ((metrics.tSec - m_minuteCandleStartSec) >= kMinuteCandleDurationSec &&
        m_minuteCandleSamples > 0)
    {
        auto& candle = minuteCandleBuf[static_cast<std::size_t>(minuteCandleWrite)];
        candle.open = m_minuteCandleOpen;
        candle.close = m_minuteCandleClose;
        candle.high = m_minuteCandleHigh;
        candle.low = m_minuteCandleLow;
        candle.t0 = m_minuteCandleStartSec;
        candle.t1 = metrics.tSec;
        candle.devLufs = metrics.loudness.momentaryLufs;
        candle.devTruePeak = metrics.loudness.truePeakDbTP;
        candle.devWidth = metrics.stereo.width;
        candle.devSpectral = metrics.tonal.spectralBalance;
        candle.devTransient = metrics.dynamics.transientDensity;
        minuteCandleWrite = (minuteCandleWrite + 1) % static_cast<int>(minuteCandleBuf.size());
        m_minuteCandleActive = false;
    }

    m_minuteAvgMomentary += (metrics.loudness.momentaryLufs - m_minuteAvgMomentary) * kAverageAlpha;
    m_minuteAvgTruePeak += (metrics.loudness.truePeakDbTP - m_minuteAvgTruePeak) * kAverageAlpha;
    m_minuteAvgWidth += (metrics.stereo.width - m_minuteAvgWidth) * kAverageAlpha;
    m_minuteAvgTilt += (metrics.tonal.spectralBalance - m_minuteAvgTilt) * kAverageAlpha;
    m_minuteAvgTransient +=
        (metrics.dynamics.transientDensity - m_minuteAvgTransient) * kAverageAlpha;
}

void UiModel::updateFrom(const AnalysisFrame& f)
{
    const auto metrics = MixMetrics::fromAnalysisFrame(f);
    const auto report = DiagnosticEngine::interpret(metrics);
    updateFrom(metrics, report);
}

void UiModel::updateFrom(const MixMetrics& metrics, const AudioSynthReport& report)
{
    lastMetrics = metrics;
    lastReport = report;

    behaviorIndex.setTargetValue(clamp01(metrics.scoring.behaviorIndex));
    mixAlignment.setTargetValue(clamp01(metrics.scoring.mixAlignment));
    signalClarity.setTargetValue(clamp01(metrics.scoring.signalClarity));
    signalStability.setTargetValue(clamp01(metrics.scoring.signalStability));

    tone.setTargetValue(clamp01(metrics.scoring.tone));
    dynamics.setTargetValue(clamp01(metrics.scoring.dynamics));
    space.setTargetValue(clamp01(metrics.scoring.space));
    punch.setTargetValue(clamp01(metrics.scoring.punch));
    balance.setTargetValue(clamp01(metrics.scoring.balance));

    midEnergy.setTargetValue(clamp01(metrics.stereo.midEnergy));
    sideEnergy.setTargetValue(clamp01(metrics.stereo.sideEnergy));
    correlation01.setTargetValue(clamp01(0.5f * (metrics.stereo.correlation + 1.0f)));
    subMono.setTargetValue(clamp01(metrics.stereo.subMonoIntegrity));
    phaseRisk.setTargetValue(clamp01(metrics.stereo.phaseRisk));
    momentaryLUFS = metrics.loudness.momentaryLufs;
    truePeakDbTP = metrics.loudness.truePeakDbTP;
    samplePeakDbFS = metrics.loudness.samplePeakDbFS;
    rmsDbFS = metrics.loudness.rmsDbFS;
    referenceTruePeakDbTP = metrics.loudness.referenceTruePeakDbTP;
    truePeakVsReferenceDb = metrics.loudness.truePeakVsReferenceDb;
    integratedLUFS = metrics.loudness.integratedLufs;
    shortTermLUFS = metrics.loudness.shortTermLufs;
    crestFactorDb = metrics.dynamics.crestFactorDb;
    stereoWidth = metrics.stereo.width;
    correlation = metrics.stereo.correlation;
    spectralBalance = metrics.tonal.spectralBalance;
    transientDensity = metrics.dynamics.transientDensity;
    subBandDb = metrics.tonal.subBandDb;
    lowBandDb = metrics.tonal.lowBandDb;
    lowMidBandDb = metrics.tonal.lowMidBandDb;
    midBandDb = metrics.tonal.midBandDb;
    highMidBandDb = metrics.tonal.highMidBandDb;
    highBandDb = metrics.tonal.highBandDb;
    presenceBandDb = metrics.tonal.presenceBandDb;
    airBandDb = metrics.tonal.airBandDb;
    referenceSubBandDb = metrics.tonal.referenceSubBandDb;
    referenceLowBandDb = metrics.tonal.referenceLowBandDb;
    referenceLowMidBandDb = metrics.tonal.referenceLowMidBandDb;
    referenceMidBandDb = metrics.tonal.referenceMidBandDb;
    referenceHighMidBandDb = metrics.tonal.referenceHighMidBandDb;
    referenceHighBandDb = metrics.tonal.referenceHighBandDb;
    referencePresenceBandDb = metrics.tonal.referencePresenceBandDb;
    referenceAirBandDb = metrics.tonal.referenceAirBandDb;
    subBandDeltaDb = metrics.referenceDeltas.subBandDb;
    lowBandDeltaDb = metrics.referenceDeltas.lowBandDb;
    lowMidBandDeltaDb = metrics.referenceDeltas.lowMidBandDb;
    midBandDeltaDb = metrics.referenceDeltas.midBandDb;
    highMidBandDeltaDb = metrics.referenceDeltas.highMidBandDb;
    highBandDeltaDb = metrics.referenceDeltas.highBandDb;
    presenceBandDeltaDb = metrics.referenceDeltas.presenceBandDb;
    airBandDeltaDb = metrics.referenceDeltas.airBandDb;

    const float lufsNorm = clamp01((metrics.loudness.integratedLufs + 24.0f) / 24.0f);
    const float crestNorm = clamp01((metrics.dynamics.crestFactorDb - 3.0f) / 12.0f);
    const float widthNorm = clamp01(metrics.stereo.width);
    const float corrNorm = clamp01(0.5f * (metrics.stereo.correlation + 1.0f));
    const float transientNorm = clamp01(metrics.dynamics.transientDensity);
    const float clarityNorm = clamp01(metrics.scoring.signalClarity);
    const float userSigTarget = clamp01(0.22f * lufsNorm + 0.22f * crestNorm + 0.20f * widthNorm +
                                        0.18f * corrNorm + 0.18f * transientNorm +
                                        0.10f * clarityNorm);

    const float baseline = (firstSessionMixAverage < 0.0f) ? 0.5f : firstSessionMixAverage;
    const float refPeakNorm = (metrics.loudness.referenceTruePeakDbTP > -80.0f)
                                  ? clamp01((metrics.loudness.referenceTruePeakDbTP + 6.0f) / 6.0f)
                                  : 0.58f;
    const float refSigTarget = clamp01(0.72f * baseline + 0.28f * refPeakNorm);
    userMixSignature.setTargetValue(userSigTarget);
    refMixSignature.setTargetValue(refSigTarget);

    detectedGenre = genreNameFromIndex(metrics.genreIndex);
    detectedGenreStability = clamp01(metrics.genreConfidence);
    lastFlags = metrics.flags;
    tonalSummary = juce::String(report.tonalSummary);
    haloLabel = juce::String(report.haloLabel);
    haloDetail = juce::String(report.haloDetail);
    statusText = juce::String(report.summaryStatus);

    diagnosticFixList.clear();
    diagnosticFixList.reserve(report.issues.size());
    for (const auto& issue : report.issues)
    {
        diagnosticFixList.push_back({
            juce::String(issue.id),
            juce::String(issue.title),
            juce::String(issue.explanation),
            juce::String(issue.suggestedAction),
            juce::String(issue.impact),
            issue.confidence});
    }

    if (fixList.size() > 7)
    {
        fixList.erase(fixList.begin(), fixList.begin() + static_cast<long>(fixList.size() - 7));
    }

    pushCandleFrom(metrics);
    pushMinuteCandleFrom(metrics);
    persistCandlesIfNeeded();
}

void UiModel::advance()
{
    (void)behaviorIndex.getNextValue();
    (void)mixAlignment.getNextValue();
    (void)signalClarity.getNextValue();
    (void)signalStability.getNextValue();
    (void)userMixSignature.getNextValue();
    (void)refMixSignature.getNextValue();
    (void)tone.getNextValue();
    (void)dynamics.getNextValue();
    (void)space.getNextValue();
    (void)punch.getNextValue();
    (void)balance.getNextValue();
    (void)midEnergy.getNextValue();
    (void)sideEnergy.getNextValue();
    (void)correlation01.getNextValue();
    (void)subMono.getNextValue();
    (void)phaseRisk.getNextValue();

    const float energy =
        clamp01(0.45f * userMixSignature.getCurrentValue() +
                0.25f * behaviorIndex.getCurrentValue() + 0.20f * sideEnergy.getCurrentValue() +
                0.10f * (1.0f - phaseRisk.getCurrentValue()));
    visualPhase += 0.05f + energy * 0.03f;
    if (visualPhase > juce::MathConstants<float>::twoPi)
    {
        visualPhase -= juce::MathConstants<float>::twoPi;
    }

    const float shimmer = 0.5f + 0.5f * std::sin(visualPhase * 1.3f);
    visualPulse = clamp01(0.65f * shimmer + 0.35f * energy);
}
