#include "UiModel.h"

#include "../Interpretation/DiagnosticEngine.h"
#include "dawai/aifr3d_core/session_store.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

namespace
{
void setSmooth(juce::SmoothedValue<float>& v, double srHz, double attackMs, double releaseMs)
{
    const double weightedMs = juce::jmax(80.0, attackMs * 0.35 + releaseMs * 0.65);
    const double rampSec = juce::jlimit(0.08, 1.20, weightedMs / 1000.0);
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

float lufsPrice(float valueLufs)
{
    return clampPrice((valueLufs + 36.0f) / 30.0f);
}

bool usableBandValue(float value)
{
    return std::isfinite(value) && value > -80.0f;
}

float finiteOr(float value, float fallback)
{
    return std::isfinite(value) ? value : fallback;
}

float ballisticFollow(float current, float target, double uiHz, float attackMs, float releaseMs)
{
    if (!std::isfinite(target))
    {
        return current;
    }

    if (!std::isfinite(current))
    {
        return target;
    }

    const double rate = uiHz > 1.0 ? uiHz : 60.0;
    const double attackTau = std::max(0.001, static_cast<double>(attackMs) * 0.001);
    const double releaseTau = std::max(0.001, static_cast<double>(releaseMs) * 0.001);
    const float attackAlpha = static_cast<float>(1.0 - std::exp(-1.0 / (rate * attackTau)));
    const float releaseAlpha = static_cast<float>(1.0 - std::exp(-1.0 / (rate * releaseTau)));
    const float alpha = target > current ? attackAlpha : releaseAlpha;
    return current + (target - current) * alpha;
}

float meterBandValue(float valueDb)
{
    return clamp01((valueDb + 42.0f) / 42.0f);
}

float loudnessMeterValue(float shortTermLufs, float truePeakDbTP)
{
    const float lufs = clamp01((shortTermLufs + 26.0f) / 20.0f);
    const float headroom = clamp01((-truePeakDbTP) / 6.0f);
    return clamp01(0.68f * lufs + 0.32f * headroom);
}

float dynamicsMeterValue(float crestDb, float transientDensity)
{
    const float crest = clamp01((crestDb - 4.0f) / 12.0f);
    return clamp01(0.58f * crest + 0.42f * transientDensity);
}

float stereoMeterValue(float width, float correlation, float sideEnergy, float phaseRisk)
{
    const float corr = clamp01(0.5f * (correlation + 1.0f));
    const float safePhase = 1.0f - clamp01(phaseRisk);
    return clamp01(0.34f * width + 0.28f * sideEnergy + 0.24f * corr + 0.14f * safePhase);
}

float toneMeterValue(const MixMetrics& metrics)
{
    const float shape = clamp01(
        (std::abs(metrics.tonal.lowMidBandDb - metrics.tonal.midBandDb) +
         std::abs(metrics.tonal.highMidBandDb - metrics.tonal.midBandDb)) /
        20.0f);
    const float density = clamp01(0.24f * meterBandValue(metrics.tonal.subBandDb) +
                                  0.18f * meterBandValue(metrics.tonal.lowBandDb) +
                                  0.18f * meterBandValue(metrics.tonal.lowMidBandDb) +
                                  0.14f * meterBandValue(metrics.tonal.midBandDb) +
                                  0.14f * meterBandValue(metrics.tonal.highMidBandDb) +
                                  0.12f * meterBandValue(metrics.tonal.highBandDb));
    return clamp01(0.56f * density + 0.44f * shape);
}

MixMetrics sanitizedMetrics(MixMetrics metrics)
{
    metrics.loudness.integratedLufs = finiteOr(metrics.loudness.integratedLufs, -99.0f);
    metrics.loudness.shortTermLufs = finiteOr(metrics.loudness.shortTermLufs, -99.0f);
    metrics.loudness.momentaryLufs = finiteOr(metrics.loudness.momentaryLufs, -99.0f);
    metrics.loudness.truePeakDbTP = finiteOr(metrics.loudness.truePeakDbTP, -99.0f);
    metrics.loudness.integratedReferenceLufs =
        finiteOr(metrics.loudness.integratedReferenceLufs, -14.0f);
    metrics.loudness.referenceTruePeakDbTP =
        finiteOr(metrics.loudness.referenceTruePeakDbTP, -99.0f);
    metrics.loudness.targetTruePeakDbTP = finiteOr(metrics.loudness.targetTruePeakDbTP, -99.0f);
    metrics.loudness.truePeakVsReferenceDb =
        finiteOr(metrics.loudness.truePeakVsReferenceDb, std::numeric_limits<float>::quiet_NaN());

    metrics.stereo.width = clamp01(metrics.stereo.width);
    metrics.stereo.correlation =
        juce::jlimit(-1.0f, 1.0f, finiteOr(metrics.stereo.correlation, 0.0f));
    metrics.stereo.midEnergy = clamp01(metrics.stereo.midEnergy);
    metrics.stereo.sideEnergy = clamp01(metrics.stereo.sideEnergy);
    metrics.stereo.leftEnergy = clamp01(metrics.stereo.leftEnergy);
    metrics.stereo.rightEnergy = clamp01(metrics.stereo.rightEnergy);
    metrics.stereo.subMonoIntegrity = clamp01(metrics.stereo.subMonoIntegrity);
    metrics.stereo.phaseRisk = clamp01(metrics.stereo.phaseRisk);
    metrics.stereo.lowBandCorrelation =
        juce::jlimit(-1.0f, 1.0f, finiteOr(metrics.stereo.lowBandCorrelation, 1.0f));
    metrics.stereo.lowBandPhaseRisk = clamp01(metrics.stereo.lowBandPhaseRisk);
    metrics.stereo.centerDominance = clamp01(metrics.stereo.centerDominance);
    metrics.stereo.sideDominance = clamp01(metrics.stereo.sideDominance);
    metrics.stereo.stereoMotion = clamp01(metrics.stereo.stereoMotion);
    metrics.stereo.spatialSpread = clamp01(metrics.stereo.spatialSpread);
    metrics.stereo.spatialW = clamp01(metrics.stereo.spatialW);
    metrics.stereo.spatialX = clamp01(metrics.stereo.spatialX);
    metrics.stereo.spatialY = clamp01(metrics.stereo.spatialY);
    metrics.stereo.spatialZ = clamp01(metrics.stereo.spatialZ);
    metrics.stereo.referenceWidth = clamp01(metrics.stereo.referenceWidth);
    metrics.stereo.referenceCorrelation =
        juce::jlimit(-1.0f, 1.0f, finiteOr(metrics.stereo.referenceCorrelation, 0.0f));
    metrics.stereo.targetWidth = clamp01(metrics.stereo.targetWidth);
    metrics.stereo.targetCorrelation =
        juce::jlimit(-1.0f, 1.0f, finiteOr(metrics.stereo.targetCorrelation, 0.0f));

    metrics.dynamics.peakDbFs = finiteOr(metrics.dynamics.peakDbFs, -99.0f);
    metrics.dynamics.crestFactorDb = finiteOr(metrics.dynamics.crestFactorDb, 0.0f);
    metrics.dynamics.dynamicRangeDb = finiteOr(metrics.dynamics.dynamicRangeDb, 0.0f);
    metrics.dynamics.dynamicRangeReferenceDb =
        finiteOr(metrics.dynamics.dynamicRangeReferenceDb, 8.0f);
    metrics.dynamics.transientDensity = clamp01(metrics.dynamics.transientDensity);
    metrics.dynamics.transientRateHz = juce::jmax(0.0f, finiteOr(metrics.dynamics.transientRateHz, 0.0f));
    metrics.dynamics.volatilityIndex = clamp01(metrics.dynamics.volatilityIndex);
    metrics.dynamics.dynOpen = clamp01(metrics.dynamics.dynOpen);
    metrics.dynamics.dynClose = clamp01(metrics.dynamics.dynClose);
    metrics.dynamics.dynHigh = clamp01(metrics.dynamics.dynHigh);
    metrics.dynamics.dynLow = clamp01(metrics.dynamics.dynLow);

    metrics.tonal.spectralBalance = finiteOr(metrics.tonal.spectralBalance, 0.0f);
    metrics.tonal.subBandDb = finiteOr(metrics.tonal.subBandDb, -90.0f);
    metrics.tonal.lowBandDb = finiteOr(metrics.tonal.lowBandDb, -90.0f);
    metrics.tonal.lowMidBandDb = finiteOr(metrics.tonal.lowMidBandDb, -90.0f);
    metrics.tonal.midBandDb = finiteOr(metrics.tonal.midBandDb, -90.0f);
    metrics.tonal.highMidBandDb = finiteOr(metrics.tonal.highMidBandDb, -90.0f);
    metrics.tonal.highBandDb = finiteOr(metrics.tonal.highBandDb, -90.0f);
    metrics.tonal.presenceBandDb = finiteOr(metrics.tonal.presenceBandDb, -90.0f);
    metrics.tonal.airBandDb = finiteOr(metrics.tonal.airBandDb, -90.0f);
    metrics.tonal.referenceSubBandDb = finiteOr(metrics.tonal.referenceSubBandDb, -90.0f);
    metrics.tonal.referenceLowBandDb = finiteOr(metrics.tonal.referenceLowBandDb, -90.0f);
    metrics.tonal.referenceLowMidBandDb = finiteOr(metrics.tonal.referenceLowMidBandDb, -90.0f);
    metrics.tonal.referenceMidBandDb = finiteOr(metrics.tonal.referenceMidBandDb, -90.0f);
    metrics.tonal.referenceHighMidBandDb =
        finiteOr(metrics.tonal.referenceHighMidBandDb, -90.0f);
    metrics.tonal.referenceHighBandDb = finiteOr(metrics.tonal.referenceHighBandDb, -90.0f);
    metrics.tonal.referencePresenceBandDb =
        finiteOr(metrics.tonal.referencePresenceBandDb, -90.0f);
    metrics.tonal.referenceAirBandDb = finiteOr(metrics.tonal.referenceAirBandDb, -90.0f);

    metrics.referenceDeltas.subBandDb =
        finiteOr(metrics.referenceDeltas.subBandDb, std::numeric_limits<float>::quiet_NaN());
    metrics.referenceDeltas.lowBandDb =
        finiteOr(metrics.referenceDeltas.lowBandDb, std::numeric_limits<float>::quiet_NaN());
    metrics.referenceDeltas.lowMidBandDb =
        finiteOr(metrics.referenceDeltas.lowMidBandDb, std::numeric_limits<float>::quiet_NaN());
    metrics.referenceDeltas.midBandDb =
        finiteOr(metrics.referenceDeltas.midBandDb, std::numeric_limits<float>::quiet_NaN());
    metrics.referenceDeltas.highMidBandDb =
        finiteOr(metrics.referenceDeltas.highMidBandDb, std::numeric_limits<float>::quiet_NaN());
    metrics.referenceDeltas.highBandDb =
        finiteOr(metrics.referenceDeltas.highBandDb, std::numeric_limits<float>::quiet_NaN());
    metrics.referenceDeltas.presenceBandDb =
        finiteOr(metrics.referenceDeltas.presenceBandDb, std::numeric_limits<float>::quiet_NaN());
    metrics.referenceDeltas.airBandDb =
        finiteOr(metrics.referenceDeltas.airBandDb, std::numeric_limits<float>::quiet_NaN());

    metrics.frequencyBalance.subEnergy = finiteOr(metrics.frequencyBalance.subEnergy, -90.0f);
    metrics.frequencyBalance.bassEnergy = finiteOr(metrics.frequencyBalance.bassEnergy, -90.0f);
    metrics.frequencyBalance.lowMidEnergy = finiteOr(metrics.frequencyBalance.lowMidEnergy, -90.0f);
    metrics.frequencyBalance.midEnergy = finiteOr(metrics.frequencyBalance.midEnergy, -90.0f);
    metrics.frequencyBalance.highMidEnergy =
        finiteOr(metrics.frequencyBalance.highMidEnergy, -90.0f);
    metrics.frequencyBalance.presenceEnergy =
        finiteOr(metrics.frequencyBalance.presenceEnergy, -90.0f);
    metrics.frequencyBalance.airEnergy = finiteOr(metrics.frequencyBalance.airEnergy, -90.0f);
    metrics.frequencyBalance.targetSub = finiteOr(metrics.frequencyBalance.targetSub, -90.0f);
    metrics.frequencyBalance.targetBass = finiteOr(metrics.frequencyBalance.targetBass, -90.0f);
    metrics.frequencyBalance.targetLowMid =
        finiteOr(metrics.frequencyBalance.targetLowMid, -90.0f);
    metrics.frequencyBalance.targetMid = finiteOr(metrics.frequencyBalance.targetMid, -90.0f);
    metrics.frequencyBalance.targetHighMid =
        finiteOr(metrics.frequencyBalance.targetHighMid, -90.0f);
    metrics.frequencyBalance.targetPresence =
        finiteOr(metrics.frequencyBalance.targetPresence, -90.0f);
    metrics.frequencyBalance.targetAir = finiteOr(metrics.frequencyBalance.targetAir, -90.0f);

    metrics.scoring.behaviorIndex = clamp01(metrics.scoring.behaviorIndex);
    metrics.scoring.mixAlignment = clamp01(metrics.scoring.mixAlignment);
    metrics.scoring.signalClarity = clamp01(metrics.scoring.signalClarity);
    metrics.scoring.signalStability = clamp01(metrics.scoring.signalStability);
    metrics.scoring.tone = clamp01(metrics.scoring.tone);
    metrics.scoring.dynamics = clamp01(metrics.scoring.dynamics);
    metrics.scoring.space = clamp01(metrics.scoring.space);
    metrics.scoring.punch = clamp01(metrics.scoring.punch);
    metrics.scoring.balance = clamp01(metrics.scoring.balance);

    metrics.genreConfidence = clamp01(metrics.genreConfidence);
    return metrics;
}

juce::String reportSignature(const AifredReport& report)
{
    juce::String signature(report.analysisState);
    for (const auto& issue : report.issues)
    {
        signature << "|" << issue.id << ":" << issue.impact;
    }
    return signature;
}

juce::String compareSnapshotLabel(const MixMetrics& metrics, const juce::String& requested)
{
    if (requested.isNotEmpty())
    {
        return requested;
    }

    juce::String label("Live capture");
    if (metrics.tSec > 0.0)
    {
        label << " @" << juce::String(metrics.tSec, 1) << "s";
    }
    return label;
}

} // namespace

void UiModel::loadPersistentCandles()
{
    m_memoryLoaded = true;
    dawai::aifr3d_core::SessionStore store(m_candleStatePath, kSessionCandleCount);
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

    dawai::aifr3d_core::SessionStore store(m_candleStatePath, kSessionCandleCount);
    dawai::aifr3d_core::SessionStoreData state;
    state.firstSessionBaseline = firstSessionMixAverage;
    state.maxSessions = kSessionCandleCount;
    state.writeIndex = static_cast<std::size_t>(std::max(0, candleWrite));
    state.candles.reserve(candleBuf.size());

    for (const auto& candle : candleBuf)
    {
        dawai::aifr3d_core::SessionCandleRecord record;
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
    m_uiHz = uiHz > 1.0 ? uiHz : 60.0;

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
    constexpr double kRealtimeCandleWindowSec = 0.75;
    constexpr double kRealtimeSilenceTimeoutSec = 0.35;
    constexpr float kActivityLufsThreshold = -62.0f;
    constexpr float kActivityTransientThreshold = 0.015f;

    const float signatureDelta = juce::jlimit(
        -1.0f, 1.0f, userMixSignature.getTargetValue() - refMixSignature.getTargetValue());
    const float signaturePrice = clampPrice(0.5f + signatureDelta * 0.28f);
    const float realtimePrice = lufsPrice(metrics.loudness.momentaryLufs);
    const bool isActiveFrame = (metrics.loudness.shortTermLufs > kActivityLufsThreshold) ||
                               (metrics.dynamics.transientDensity > kActivityTransientThreshold);

    if (!m_realtimeCandleActive && isActiveFrame)
    {
        m_realtimeCandleStartSec = metrics.tSec;
        m_realtimeLastActiveSec = metrics.tSec;
        m_realtimeOpen = realtimePrice;
        m_realtimeClose = realtimePrice;
        m_realtimeHigh = realtimePrice;
        m_realtimeLow = realtimePrice;
        m_realtimeFlags = metrics.flags;
        m_realtimeSamples = 0;
        m_realtimeCandleActive = true;
        liveBufferCandleActive = true;
        liveBufferCandle = {};
        liveBufferCandle.open = realtimePrice;
        liveBufferCandle.close = realtimePrice;
        liveBufferCandle.high = realtimePrice;
        liveBufferCandle.low = realtimePrice;
        liveBufferCandle.flags = metrics.flags;
        liveBufferCandle.t0 = metrics.tSec;
        liveBufferCandle.t1 = metrics.tSec;
        liveBufferCandle.devLufs = metrics.loudness.momentaryLufs;
        liveBufferCandle.devTruePeak = metrics.loudness.truePeakDbTP;
        liveBufferCandle.devCrest = metrics.dynamics.crestFactorDb;
        liveBufferCandle.devSpectral = metrics.tonal.spectralBalance;
        liveBufferCandle.devWidth = metrics.stereo.width;
        liveBufferCandle.devCorrelation = metrics.stereo.correlation;
        liveBufferCandle.devTransient = metrics.dynamics.transientRateHz;
    }

    if (m_realtimeCandleActive)
    {
        if (isActiveFrame)
        {
            m_realtimeLastActiveSec = metrics.tSec;
            m_realtimeClose = realtimePrice;
            m_realtimeHigh = juce::jmax(m_realtimeHigh, realtimePrice);
            m_realtimeLow = juce::jmin(m_realtimeLow, realtimePrice);
            m_realtimeFlags |= metrics.flags;
            ++m_realtimeSamples;

            liveBufferCandleActive = true;
            liveBufferCandle.open = m_realtimeOpen;
            liveBufferCandle.close = m_realtimeClose;
            liveBufferCandle.high = m_realtimeHigh;
            liveBufferCandle.low = m_realtimeLow;
            liveBufferCandle.flags = m_realtimeFlags;
            liveBufferCandle.t0 = m_realtimeCandleStartSec;
            liveBufferCandle.t1 = metrics.tSec;
            liveBufferCandle.devLufs = metrics.loudness.momentaryLufs;
            liveBufferCandle.devTruePeak = metrics.loudness.truePeakDbTP;
            liveBufferCandle.devCrest = metrics.dynamics.crestFactorDb;
            liveBufferCandle.devSpectral = metrics.tonal.spectralBalance;
            liveBufferCandle.devWidth = metrics.stereo.width;
            liveBufferCandle.devCorrelation = metrics.stereo.correlation;
            liveBufferCandle.devTransient = metrics.dynamics.transientRateHz;
        }

        const bool realtimeTimedOut =
            !isActiveFrame &&
            ((metrics.tSec - m_realtimeLastActiveSec) >= kRealtimeSilenceTimeoutSec);
        const bool realtimeWindowComplete =
            (metrics.tSec - m_realtimeCandleStartSec) >= kRealtimeCandleWindowSec;

        if (realtimeTimedOut || realtimeWindowComplete)
        {
            Candle committed;
            committed.open = m_realtimeOpen;
            committed.close = m_realtimeClose;
            committed.high = m_realtimeHigh;
            committed.low = m_realtimeLow;
            committed.flags = m_realtimeFlags;
            committed.t0 = m_realtimeCandleStartSec;
            committed.t1 = m_realtimeLastActiveSec;
            committed.devLufs = metrics.loudness.momentaryLufs;
            committed.devTruePeak = metrics.loudness.truePeakDbTP;
            committed.devCrest = metrics.dynamics.crestFactorDb;
            committed.devSpectral = metrics.tonal.spectralBalance;
            committed.devWidth = metrics.stereo.width;
            committed.devCorrelation = metrics.stereo.correlation;
            committed.devTransient = metrics.dynamics.transientRateHz;

            realtimeCandleBuf[static_cast<std::size_t>(realtimeCandleWrite)] = committed;
            realtimeCandleWrite =
                (realtimeCandleWrite + 1) % static_cast<int>(realtimeCandleBuf.size());
            liveBufferCandle = committed;
            liveBufferCandleActive = false;
            m_realtimeCandleActive = false;

            if (isActiveFrame)
            {
                m_realtimeCandleStartSec = metrics.tSec;
                m_realtimeLastActiveSec = metrics.tSec;
                m_realtimeOpen = realtimePrice;
                m_realtimeClose = realtimePrice;
                m_realtimeHigh = realtimePrice;
                m_realtimeLow = realtimePrice;
                m_realtimeFlags = metrics.flags;
                m_realtimeSamples = 1;
                m_realtimeCandleActive = true;
                liveBufferCandleActive = true;
                liveBufferCandle = committed;
                liveBufferCandle.open = realtimePrice;
                liveBufferCandle.close = realtimePrice;
                liveBufferCandle.high = realtimePrice;
                liveBufferCandle.low = realtimePrice;
                liveBufferCandle.flags = metrics.flags;
                liveBufferCandle.t0 = metrics.tSec;
                liveBufferCandle.t1 = metrics.tSec;
            }
        }
    }

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
        m_sessionCrestSum = 0.0f;
        m_sessionSpectralSum = 0.0f;
        m_sessionWidthSum = 0.0f;
        m_sessionCorrSum = 0.0f;
        m_sessionTransientSum = 0.0f;
        m_sessionActive = true;
        liveSessionActive = true;
        liveSessionReferenceDelta = signatureDelta;
        liveSessionCandle = {};
        liveSessionCandle.open = m_sessionOpen;
        liveSessionCandle.close = m_sessionClose;
        liveSessionCandle.high = m_sessionHigh;
        liveSessionCandle.low = m_sessionLow;
        liveSessionCandle.flags = m_sessionFlags;
        liveSessionCandle.t0 = m_sessionStartSec;
    }

    if (!m_sessionActive)
    {
        liveSessionActive = false;
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
        m_sessionCrestSum += metrics.dynamics.crestFactorDb;
        m_sessionSpectralSum += metrics.tonal.spectralBalance;
        m_sessionWidthSum += metrics.stereo.width;
        m_sessionCorrSum += metrics.stereo.correlation;
        m_sessionTransientSum += metrics.dynamics.transientDensity;

        liveSessionActive = true;
        liveSessionReferenceDelta = signatureDelta;
        liveSessionCandle.open = m_sessionOpen;
        liveSessionCandle.close = m_sessionClose;
        liveSessionCandle.high = m_sessionHigh;
        liveSessionCandle.low = m_sessionLow;
        liveSessionCandle.flags = m_sessionFlags;
        liveSessionCandle.t0 = m_sessionStartSec;
        liveSessionCandle.t1 = m_lastActiveSec;
        liveSessionCandle.devLufs = metrics.loudness.integratedLufs;
        liveSessionCandle.devTruePeak = metrics.loudness.truePeakDbTP;
        liveSessionCandle.devCrest = metrics.dynamics.crestFactorDb;
        liveSessionCandle.devSpectral = metrics.tonal.spectralBalance;
        liveSessionCandle.devWidth = metrics.stereo.width;
        liveSessionCandle.devCorrelation = metrics.stereo.correlation;
        liveSessionCandle.devTransient = metrics.dynamics.transientDensity;
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
        liveSessionActive = false;
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
    liveSessionActive = false;
    liveSessionReferenceDelta = lastReferenceDelta;
    liveSessionCandle = committed;
}

void UiModel::updateFrom(const AnalysisFrame& f)
{
    const auto metrics = MixMetrics::fromAnalysisFrame(f);
    const auto report = DiagnosticEngine::interpret(metrics);
    updateFrom(metrics, report);
}

void UiModel::updateFrom(const MixMetrics& metrics, const AifredReport& report)
{
    lastMetrics = sanitizedMetrics(metrics);
    const auto& sanitized = lastMetrics;
    lastReport = report;
    analysisStateCode = sanitized.analysisStateCode;
    referenceStateCode = sanitized.referenceStateCode;
    hasReferenceData =
        (sanitized.referenceStateCode == kReferenceStateReady) && sanitized.referenceValid;
    hasScoredResult = sanitized.scoredResultValid;
    hasLiveSignal = sanitized.liveSignalPresent;
    hasValidSpectralData = usableBandValue(sanitized.tonal.subBandDb) ||
                           usableBandValue(sanitized.tonal.lowBandDb) ||
                           usableBandValue(sanitized.tonal.lowMidBandDb) ||
                           usableBandValue(sanitized.tonal.midBandDb) ||
                           usableBandValue(sanitized.tonal.highMidBandDb) ||
                           usableBandValue(sanitized.tonal.highBandDb);

    behaviorIndex.setTargetValue(hasScoredResult ? sanitized.scoring.behaviorIndex : 0.0f);
    mixAlignment.setTargetValue(hasScoredResult ? sanitized.scoring.mixAlignment : 0.0f);
    signalClarity.setTargetValue(hasScoredResult ? sanitized.scoring.signalClarity : 0.0f);
    signalStability.setTargetValue(hasScoredResult ? sanitized.scoring.signalStability : 0.0f);

    tone.setTargetValue(hasScoredResult ? sanitized.scoring.tone : 0.0f);
    dynamics.setTargetValue(hasScoredResult ? sanitized.scoring.dynamics : 0.0f);
    space.setTargetValue(hasScoredResult ? sanitized.scoring.space : 0.0f);
    punch.setTargetValue(hasScoredResult ? sanitized.scoring.punch : 0.0f);
    balance.setTargetValue(hasScoredResult ? sanitized.scoring.balance : 0.0f);

    midEnergy.setTargetValue(hasLiveSignal ? sanitized.stereo.midEnergy : 0.5f);
    sideEnergy.setTargetValue(hasLiveSignal ? sanitized.stereo.sideEnergy : 0.0f);
    correlation01.setTargetValue(hasLiveSignal ? clamp01(0.5f * (sanitized.stereo.correlation + 1.0f))
                                               : 0.5f);
    subMono.setTargetValue(hasLiveSignal ? sanitized.stereo.subMonoIntegrity : 0.5f);
    phaseRisk.setTargetValue(hasLiveSignal ? sanitized.stereo.phaseRisk : 0.0f);
    truePeakDbTP = sanitized.loudness.truePeakDbTP;
    peakDbFS = sanitized.dynamics.peakDbFs;
    referenceTruePeakDbTP = sanitized.loudness.referenceTruePeakDbTP;
    truePeakVsReferenceDb = sanitized.loudness.truePeakVsReferenceDb;
    integratedLUFS = sanitized.loudness.integratedLufs;
    shortTermLUFS = sanitized.loudness.shortTermLufs;
    momentaryLUFS = sanitized.loudness.momentaryLufs;
    crestFactorDb = sanitized.dynamics.crestFactorDb;
    stereoWidth = sanitized.stereo.width;
    correlation = sanitized.stereo.correlation;
    spectralBalance = sanitized.tonal.spectralBalance;
    transientDensity = sanitized.dynamics.transientDensity;
    transientRateHz = sanitized.dynamics.transientRateHz;
    volatilityIndex = sanitized.dynamics.volatilityIndex;
    subBandDb = sanitized.tonal.subBandDb;
    lowBandDb = sanitized.tonal.lowBandDb;
    lowMidBandDb = sanitized.tonal.lowMidBandDb;
    midBandDb = sanitized.tonal.midBandDb;
    highMidBandDb = sanitized.tonal.highMidBandDb;
    highBandDb = sanitized.tonal.highBandDb;
    presenceBandDb = sanitized.tonal.presenceBandDb;
    airBandDb = sanitized.tonal.airBandDb;
    referenceSubBandDb = sanitized.tonal.referenceSubBandDb;
    referenceLowBandDb = sanitized.tonal.referenceLowBandDb;
    referenceLowMidBandDb = sanitized.tonal.referenceLowMidBandDb;
    referenceMidBandDb = sanitized.tonal.referenceMidBandDb;
    referenceHighMidBandDb = sanitized.tonal.referenceHighMidBandDb;
    referenceHighBandDb = sanitized.tonal.referenceHighBandDb;
    referencePresenceBandDb = sanitized.tonal.referencePresenceBandDb;
    referenceAirBandDb = sanitized.tonal.referenceAirBandDb;
    subBandDeltaDb = sanitized.referenceDeltas.subBandDb;
    lowBandDeltaDb = sanitized.referenceDeltas.lowBandDb;
    lowMidBandDeltaDb = sanitized.referenceDeltas.lowMidBandDb;
    midBandDeltaDb = sanitized.referenceDeltas.midBandDb;
    highMidBandDeltaDb = sanitized.referenceDeltas.highMidBandDb;
    highBandDeltaDb = sanitized.referenceDeltas.highBandDb;
    presenceBandDeltaDb = sanitized.referenceDeltas.presenceBandDb;
    airBandDeltaDb = sanitized.referenceDeltas.airBandDb;

    const float lufsNorm = clamp01((sanitized.loudness.integratedLufs + 24.0f) / 24.0f);
    const float crestNorm = clamp01((sanitized.dynamics.crestFactorDb - 3.0f) / 12.0f);
    const float widthNorm = sanitized.stereo.width;
    const float corrNorm = clamp01(0.5f * (sanitized.stereo.correlation + 1.0f));
    const float transientNorm = sanitized.dynamics.transientDensity;
    const float clarityNorm = sanitized.scoring.signalClarity;
    const float userSigTarget =
        hasLiveSignal
            ? clamp01(0.22f * lufsNorm + 0.22f * crestNorm + 0.20f * widthNorm +
                      0.18f * corrNorm + 0.18f * transientNorm + 0.10f * clarityNorm)
            : userMixSignature.getCurrentValue();

    const float baseline = (firstSessionMixAverage < 0.0f) ? 0.5f : firstSessionMixAverage;
    const float refPeakNorm = (sanitized.loudness.referenceTruePeakDbTP > -80.0f)
                                  ? clamp01((sanitized.loudness.referenceTruePeakDbTP + 6.0f) / 6.0f)
                                  : 0.58f;
    const float refSigTarget =
        hasReferenceData ? clamp01(0.72f * baseline + 0.28f * refPeakNorm)
                         : refMixSignature.getCurrentValue();
    userMixSignature.setTargetValue(userSigTarget);
    refMixSignature.setTargetValue(refSigTarget);

    detectedGenre = genreNameFromIndex(sanitized.genreIndex);
    detectedGenreStability = sanitized.genreConfidence;
    lastFlags = sanitized.flags;
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

    const auto nextFixSignature = reportSignature(report);
    const bool liveInterpretationWindow =
        hasLiveSignal &&
        (analysisStateCode == kAnalysisStateActiveLive ||
         analysisStateCode == kAnalysisStateValidScored);
    const bool stateRequiresImmediateFixSwap =
        referenceStateCode == kReferenceStateBlocked;

    if (stateRequiresImmediateFixSwap)
    {
        fixList = diagnosticFixList;
        m_appliedFixSignature = nextFixSignature;
        m_pendingFixSignature.clear();
        m_fixHoldFrames = 0;
    }
    else if (liveInterpretationWindow)
    {
        if (nextFixSignature == m_appliedFixSignature)
        {
            if (fixList.empty())
            {
                fixList = diagnosticFixList;
            }
            m_pendingFixSignature.clear();
            m_fixHoldFrames = 0;
        }
        else
        {
            if (nextFixSignature != m_pendingFixSignature)
            {
                m_pendingFixSignature = nextFixSignature;
                m_fixHoldFrames = 1;
            }
            else
            {
                ++m_fixHoldFrames;
            }

            if (m_fixHoldFrames >= 3)
            {
                fixList = diagnosticFixList;
                m_appliedFixSignature = nextFixSignature;
                m_pendingFixSignature.clear();
                m_fixHoldFrames = 0;
            }
        }
    }
    else
    {
        m_pendingFixSignature.clear();
        m_fixHoldFrames = 0;
    }

    pushCandleFrom(sanitized);
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

    const float quietLufs = -70.0f;
    const float quietPeak = -120.0f;
    meterShortTermLUFS = ballisticFollow(meterShortTermLUFS,
                                         hasLiveSignal ? shortTermLUFS : quietLufs, m_uiHz, 110.0f,
                                         420.0f);
    meterMomentaryLUFS = ballisticFollow(meterMomentaryLUFS,
                                         hasLiveSignal ? momentaryLUFS : quietLufs, m_uiHz, 90.0f,
                                         320.0f);
    meterTruePeakDbTP = ballisticFollow(meterTruePeakDbTP,
                                        hasLiveSignal ? truePeakDbTP : quietPeak, m_uiHz, 55.0f,
                                        260.0f);
    meterPeakDbFS = ballisticFollow(meterPeakDbFS, hasLiveSignal ? peakDbFS : quietPeak, m_uiHz,
                                    55.0f, 260.0f);
    meterCrestFactorDb = ballisticFollow(meterCrestFactorDb,
                                         hasLiveSignal ? crestFactorDb : 0.0f, m_uiHz, 140.0f,
                                         520.0f);
    meterStereoWidth = ballisticFollow(meterStereoWidth,
                                       hasLiveSignal ? stereoWidth : 0.0f, m_uiHz, 130.0f, 420.0f);
    meterCorrelation = ballisticFollow(meterCorrelation,
                                       hasLiveSignal ? correlation : 0.0f, m_uiHz, 120.0f, 360.0f);
    meterVolatilityIndex = ballisticFollow(meterVolatilityIndex,
                                           hasLiveSignal ? volatilityIndex : 0.0f, m_uiHz, 180.0f,
                                           620.0f);

    const std::array<float, 8> liveBandTargets{
        hasLiveSignal ? subBandDb : -90.0f,       hasLiveSignal ? lowBandDb : -90.0f,
        hasLiveSignal ? lowMidBandDb : -90.0f,    hasLiveSignal ? midBandDb : -90.0f,
        hasLiveSignal ? highMidBandDb : -90.0f,   hasLiveSignal ? highBandDb : -90.0f,
        hasLiveSignal ? presenceBandDb : -90.0f,  hasLiveSignal ? airBandDb : -90.0f};
    const std::array<float, 8> deltaTargets{
        std::isfinite(subBandDeltaDb) ? subBandDeltaDb : 0.0f,
        std::isfinite(lowBandDeltaDb) ? lowBandDeltaDb : 0.0f,
        std::isfinite(lowMidBandDeltaDb) ? lowMidBandDeltaDb : 0.0f,
        std::isfinite(midBandDeltaDb) ? midBandDeltaDb : 0.0f,
        std::isfinite(highMidBandDeltaDb) ? highMidBandDeltaDb : 0.0f,
        std::isfinite(highBandDeltaDb) ? highBandDeltaDb : 0.0f,
        std::isfinite(presenceBandDeltaDb) ? presenceBandDeltaDb : 0.0f,
        std::isfinite(airBandDeltaDb) ? airBandDeltaDb : 0.0f};

    for (std::size_t i = 0; i < liveBandDisplayDb.size(); ++i)
    {
        liveBandDisplayDb[i] =
            ballisticFollow(liveBandDisplayDb[i], liveBandTargets[i], m_uiHz, 80.0f, 240.0f);
        referenceDeltaDisplayDb[i] = ballisticFollow(referenceDeltaDisplayDb[i],
                                                     deltaTargets[i], m_uiHz, 110.0f, 360.0f);
    }

    liveToneMeter = ballisticFollow(liveToneMeter,
                                    hasLiveSignal ? toneMeterValue(lastMetrics) : 0.0f, m_uiHz,
                                    120.0f, 360.0f);
    liveStereoMeter =
        ballisticFollow(liveStereoMeter,
                        hasLiveSignal ? stereoMeterValue(stereoWidth, correlation,
                                                         lastMetrics.stereo.sideEnergy,
                                                         lastMetrics.stereo.phaseRisk)
                                      : 0.0f,
                        m_uiHz, 110.0f, 320.0f);
    liveLoudnessMeter = ballisticFollow(
        liveLoudnessMeter,
        hasLiveSignal ? loudnessMeterValue(meterShortTermLUFS, meterTruePeakDbTP) : 0.0f, m_uiHz,
        95.0f, 260.0f);
    liveDynamicsMeter =
        ballisticFollow(liveDynamicsMeter,
                        hasLiveSignal ? dynamicsMeterValue(meterCrestFactorDb, transientDensity)
                                      : 0.0f,
                        m_uiHz, 130.0f, 420.0f);

    float energy = 0.0f;
    float phaseStep = 0.009f;
    float pulseFloor = 0.012f;
    float pulseGain = 0.020f;

    if (hasLiveSignal)
    {
        const bool warming = analysisStateCode == kAnalysisStateBufferWarming;
        const bool activeLive = analysisStateCode == kAnalysisStateActiveLive;
        const bool scored = analysisStateCode == kAnalysisStateValidScored && hasScoredResult;
        const bool liveOnly = !hasReferenceData;

        if (scored || liveOnly)
        {
            energy = clamp01(0.45f * userMixSignature.getCurrentValue() +
                             0.18f * behaviorIndex.getCurrentValue() +
                             0.14f * sideEnergy.getCurrentValue() +
                             0.13f * lastMetrics.stereo.stereoMotion +
                             0.10f * lastMetrics.stereo.spatialSpread +
                             0.10f * lastMetrics.dynamics.transientDensity +
                             0.10f * (1.0f - phaseRisk.getCurrentValue()));
            phaseStep = 0.042f + energy * 0.026f;
            pulseFloor = liveOnly ? 0.14f : 0.18f;
            pulseGain = liveOnly ? 0.28f : 0.48f;
        }
        else if (activeLive)
        {
            energy = clamp01(0.18f + 0.35f * sideEnergy.getCurrentValue() +
                             0.20f * midEnergy.getCurrentValue() +
                             0.16f * lastMetrics.stereo.stereoMotion +
                             0.12f * lastMetrics.dynamics.transientDensity +
                             0.15f * clamp01((shortTermLUFS + 30.0f) / 24.0f));
            phaseStep = 0.030f + energy * 0.022f;
            pulseFloor = 0.11f;
            pulseGain = 0.34f;
        }
        else if (warming)
        {
            energy = clamp01(0.10f + 0.25f * sideEnergy.getCurrentValue() +
                             0.20f * midEnergy.getCurrentValue() +
                             0.08f * lastMetrics.stereo.stereoMotion);
            phaseStep = 0.018f + energy * 0.012f;
            pulseFloor = 0.05f;
            pulseGain = 0.16f;
        }
        else
        {
            energy = clamp01(0.12f * sideEnergy.getCurrentValue() +
                             0.08f * midEnergy.getCurrentValue());
            phaseStep = 0.014f + energy * 0.010f;
            pulseFloor = 0.03f;
            pulseGain = 0.08f;
        }
    }

    visualPhase += phaseStep;
    if (visualPhase > juce::MathConstants<float>::twoPi)
    {
        visualPhase -= juce::MathConstants<float>::twoPi;
    }

    const float shimmer = 0.5f + 0.5f * std::sin(visualPhase * 1.3f);
    visualPulse = clamp01(pulseFloor + pulseGain * energy + 0.12f * pulseGain * shimmer);
}

void UiModel::setPresentationMode(UiPresentationMode mode) noexcept
{
    presentationMode = mode;
}

bool UiModel::captureCompareSnapshotA(const juce::String& label)
{
    if (!hasLiveSignal)
    {
        return false;
    }

    compareMixA.metrics = lastMetrics;
    compareMixA.valid = true;
    compareMixA.capturedAtSec = lastMetrics.tSec;
    compareMixA.label = compareSnapshotLabel(lastMetrics, label);
    return true;
}

bool UiModel::captureCompareSnapshotB(const juce::String& label)
{
    if (!hasLiveSignal)
    {
        return false;
    }

    compareMixB.metrics = lastMetrics;
    compareMixB.valid = true;
    compareMixB.capturedAtSec = lastMetrics.tSec;
    compareMixB.label = compareSnapshotLabel(lastMetrics, label);
    return true;
}

void UiModel::clearCompareSnapshots()
{
    compareMixA = {};
    compareMixB = {};
}
