#include "MixSignatureMeter.h"

#include "../Plugin/Theme.h"

#include <array>
#include <cmath>
#include <limits>

namespace
{
juce::Colour blueUnder()
{
    return juce::Colour(0xff3b82f6);
}

juce::Colour greenTarget()
{
    return juce::Colour(0xff39ff88);
}

float clampUnit(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

const MixMetrics* compareMixA(const UiModel* model)
{
    return (model != nullptr && model->compareMixA.valid) ? &model->compareMixA.metrics : nullptr;
}

const MixMetrics* compareMixB(const UiModel* model)
{
    if (model == nullptr)
    {
        return nullptr;
    }
    if (model->compareMixB.valid)
    {
        return &model->compareMixB.metrics;
    }
    if (model->hasLiveSignal)
    {
        return &model->lastMetrics;
    }
    return nullptr;
}

juce::String panelTitle(const UiModel* model)
{
    if (model == nullptr)
    {
        return "MIX SIGNATURE";
    }

    switch (model->presentationMode)
    {
    case UiPresentationMode::Compare:
        return "MIX SIGNATURE (A/B DELTA)";
    case UiPresentationMode::Reference:
        return "MIX SIGNATURE (REFERENCE ALIGNMENT)";
    case UiPresentationMode::Analyze:
    default:
        return "MIX SIGNATURE (LIVE SESSION)";
    }
}

juce::String panelHelper(const UiModel* model)
{
    if (model == nullptr)
    {
        return "Waiting for live program material.";
    }

    if (model->referenceStateCode == kReferenceStateBlocked)
    {
        return "Reference source was blocked. Live measurements stay active while target evaluation is disabled.";
    }

    if (model->presentationMode == UiPresentationMode::Reference && !model->hasReferenceData)
    {
        return "Reference data is unavailable. Live instruments stay active in neutral evaluation mode.";
    }

    if (!model->hasLiveSignal)
    {
        return model->analysisStateCode == kAnalysisStateBufferWarming
                   ? "Collecting enough realtime buffer for a trustworthy window."
                   : "Waiting for live program material.";
    }

    switch (model->presentationMode)
    {
    case UiPresentationMode::Compare:
        return "Mix A and Mix B fingerprints stay measurement-driven. Band rows show direct deltas, not reference drift.";
    case UiPresentationMode::Reference:
        return "Live mix is being compared against the active genre/reference corridor.";
    case UiPresentationMode::Analyze:
    default:
        return "Fingerprint, cards, and rows follow live tonal, dynamic, and stereo behavior from the realtime buffer.";
    }
}

juce::Colour metricColour(const Theme& t, float value, float low, float high)
{
    if (value <= low)
    {
        return blueUnder();
    }
    if (value <= (low + high) * 0.5f)
    {
        return t.cyan;
    }
    if (value <= high)
    {
        return greenTarget();
    }
    if (value <= high * 1.25f + 0.001f)
    {
        return t.gold;
    }
    return t.redWarn;
}

juce::Colour comparisonColour(const Theme& t, float delta, float mild, float strong)
{
    const float magnitude = std::abs(delta);
    if (magnitude <= mild * 0.35f)
    {
        return greenTarget();
    }
    if (delta <= -strong)
    {
        return blueUnder();
    }
    if (delta <= -mild)
    {
        return t.cyan;
    }
    if (delta >= strong)
    {
        return t.redWarn;
    }
    if (delta >= mild)
    {
        return t.gold;
    }
    return greenTarget();
}

juce::Colour loudnessStatusColour(const Theme& t, float value, float target)
{
    const float delta = value - target;
    if (value > -7.5f)
    {
        return t.redWarn;
    }
    if (delta <= -5.5f)
    {
        return blueUnder();
    }
    if (delta <= -2.5f)
    {
        return t.cyan;
    }
    if (std::abs(delta) <= 1.5f)
    {
        return greenTarget();
    }
    if (std::abs(delta) <= 3.0f)
    {
        return t.gold;
    }
    return delta > 0.0f ? t.redWarn : blueUnder();
}

juce::Colour truePeakStatusColour(const Theme& t, float value)
{
    if (value > -0.35f)
    {
        return t.redWarn;
    }
    if (value > -0.8f)
    {
        return t.gold;
    }
    if (value > -1.2f)
    {
        return greenTarget();
    }
    if (value > -4.0f)
    {
        return t.cyan;
    }
    return blueUnder();
}

juce::Colour widthStatusColour(const Theme& t, float value, float target)
{
    const float delta = value - target;
    if (value < 0.10f)
    {
        return blueUnder();
    }
    if (std::abs(delta) <= 0.08f)
    {
        return greenTarget();
    }
    if (std::abs(delta) <= 0.18f)
    {
        return t.gold;
    }
    if (delta < 0.0f)
    {
        return t.cyan;
    }
    return t.redWarn;
}

juce::Colour punchStatusColour(const Theme& t, float value, float target)
{
    const float delta = value - target;
    if (value < 5.0f)
    {
        return blueUnder();
    }
    if (std::abs(delta) <= 2.0f)
    {
        return greenTarget();
    }
    if (std::abs(delta) <= 4.0f)
    {
        return t.gold;
    }
    if (delta < 0.0f)
    {
        return t.cyan;
    }
    return t.redWarn;
}

juce::Colour bandEnergyColour(const Theme& t, float value)
{
    if (value <= -30.0f)
    {
        return blueUnder();
    }
    if (value <= -22.0f)
    {
        return t.cyan;
    }
    if (value <= -10.0f)
    {
        return greenTarget();
    }
    if (value <= -6.0f)
    {
        return t.gold;
    }
    return t.redWarn;
}

float bandValue(const MixMetrics& metrics, int index)
{
    switch (index)
    {
    case 0:
        return metrics.tonal.subBandDb;
    case 1:
        return metrics.tonal.lowBandDb;
    case 2:
        return metrics.tonal.lowMidBandDb;
    case 3:
        return metrics.tonal.midBandDb;
    case 4:
        return metrics.tonal.highMidBandDb;
    case 5:
        return metrics.tonal.highBandDb;
    default:
        return metrics.tonal.airBandDb;
    }
}

float referenceDelta(const UiModel* model, int index)
{
    if (model == nullptr)
    {
        return std::numeric_limits<float>::quiet_NaN();
    }
    switch (index)
    {
    case 0:
        return model->subBandDeltaDb;
    case 1:
        return model->lowBandDeltaDb;
    case 2:
        return model->lowMidBandDeltaDb;
    case 3:
        return model->midBandDeltaDb;
    case 4:
        return model->highMidBandDeltaDb;
    case 5:
        return model->highBandDeltaDb;
    default:
        return model->airBandDeltaDb;
    }
}

float normalisedDb(float value, float low = -60.0f, float high = -6.0f)
{
    return clampUnit((value - low) / (high - low));
}

std::array<juce::String, 12> fingerprintLabels()
{
    return {{"SUB", "CONTROL", "LOW-MID", "MID", "BITE", "AIR",
             "DYN", "TRNS", "WIDTH", "CENTER", "PHASE", "PRESS"}};
}

std::array<float, 12> fingerprintAxesFromMetrics(const MixMetrics& metrics)
{
    const float phaseStability =
        clampUnit(0.55f * (0.5f * (metrics.stereo.correlation + 1.0f)) +
                  0.45f * metrics.stereo.subMonoIntegrity);
    const float lowEndControl =
        clampUnit(0.60f * metrics.stereo.subMonoIntegrity +
                  0.40f * (1.0f - metrics.stereo.lowBandPhaseRisk));

    return {{
        normalisedDb(metrics.tonal.subBandDb),
        lowEndControl,
        normalisedDb(metrics.tonal.lowMidBandDb),
        normalisedDb(metrics.tonal.midBandDb),
        clampUnit((metrics.tonal.highMidBandDb - metrics.tonal.midBandDb + 12.0f) / 24.0f),
        clampUnit(0.55f * normalisedDb(metrics.tonal.airBandDb) +
                  0.45f * clampUnit((metrics.tonal.spectralBalance + 9.0f) / 18.0f)),
        clampUnit((metrics.dynamics.crestFactorDb - 3.0f) / 15.0f),
        clampUnit(metrics.dynamics.transientRateHz / 12.0f),
        clampUnit(metrics.stereo.width),
        clampUnit(metrics.stereo.centerDominance),
        phaseStability,
        clampUnit((metrics.loudness.integratedLufs + 24.0f) / 18.0f),
    }};
}

std::array<float, 12> referenceFingerprintAxes(const MixMetrics& metrics, bool& hasTarget)
{
    hasTarget = false;
    const auto targetBand = [&](float live, float delta, float referenceFallback)
    {
        if (std::isfinite(delta))
        {
            hasTarget = true;
            return live - delta;
        }
        if (std::isfinite(referenceFallback) && referenceFallback > -80.0f)
        {
            hasTarget = true;
            return referenceFallback;
        }
        return live;
    };

    MixMetrics target = metrics;
    target.tonal.subBandDb =
        targetBand(metrics.tonal.subBandDb, metrics.referenceDeltas.subBandDb,
                   metrics.tonal.referenceSubBandDb);
    target.tonal.lowBandDb =
        targetBand(metrics.tonal.lowBandDb, metrics.referenceDeltas.lowBandDb,
                   metrics.tonal.referenceLowBandDb);
    target.tonal.lowMidBandDb =
        targetBand(metrics.tonal.lowMidBandDb, metrics.referenceDeltas.lowMidBandDb,
                   metrics.tonal.referenceLowMidBandDb);
    target.tonal.midBandDb =
        targetBand(metrics.tonal.midBandDb, metrics.referenceDeltas.midBandDb,
                   metrics.tonal.referenceMidBandDb);
    target.tonal.highMidBandDb =
        targetBand(metrics.tonal.highMidBandDb, metrics.referenceDeltas.highMidBandDb,
                   metrics.tonal.referenceHighMidBandDb);
    target.tonal.highBandDb =
        targetBand(metrics.tonal.highBandDb, metrics.referenceDeltas.highBandDb,
                   metrics.tonal.referenceHighBandDb);
    target.tonal.airBandDb =
        targetBand(metrics.tonal.airBandDb, metrics.referenceDeltas.airBandDb,
                   metrics.tonal.referenceAirBandDb);

    if (std::isfinite(metrics.loudness.integratedReferenceLufs) &&
        metrics.loudness.integratedReferenceLufs > -80.0f)
    {
        target.loudness.integratedLufs = metrics.loudness.integratedReferenceLufs;
        hasTarget = true;
    }
    if (std::isfinite(metrics.dynamics.dynamicRangeReferenceDb) &&
        metrics.dynamics.dynamicRangeReferenceDb > 0.0f)
    {
        target.dynamics.crestFactorDb = metrics.dynamics.dynamicRangeReferenceDb;
        hasTarget = true;
    }
    if (metrics.stereo.targetWidth > 0.0f || std::abs(metrics.stereo.targetCorrelation) > 0.001f)
    {
        target.stereo.width = metrics.stereo.targetWidth;
        target.stereo.correlation = metrics.stereo.targetCorrelation;
        target.stereo.centerDominance = clampUnit(1.0f - metrics.stereo.targetWidth * 0.52f);
        target.stereo.lowBandPhaseRisk = 0.0f;
        target.stereo.subMonoIntegrity = clampUnit(0.5f * (metrics.stereo.targetCorrelation + 1.0f));
        hasTarget = true;
    }

    return fingerprintAxesFromMetrics(target);
}

juce::Point<float> fingerprintPoint(juce::Point<float> center, float radius, int index, int total,
                                    float value)
{
    const float angle =
        -juce::MathConstants<float>::halfPi +
        (juce::MathConstants<float>::twoPi * static_cast<float>(index) / static_cast<float>(total));
    return {center.x + std::cos(angle) * radius * value,
            center.y + std::sin(angle) * radius * value};
}

juce::Path fingerprintPath(juce::Rectangle<float> area, const std::array<float, 12>& axes,
                           float scale = 1.0f)
{
    juce::Path path;
    const auto center = area.getCentre();
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.40f * scale;

    for (int index = 0; index < static_cast<int>(axes.size()); ++index)
    {
        const auto point =
            fingerprintPoint(center, radius, index, static_cast<int>(axes.size()), axes[static_cast<std::size_t>(index)]);
        if (index == 0)
        {
            path.startNewSubPath(point);
        }
        else
        {
            path.lineTo(point);
        }
    }

    path.closeSubPath();
    return path;
}

void drawFingerprintGuides(juce::Graphics& g, juce::Rectangle<float> area, const Theme& t)
{
    const auto center = area.getCentre();
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.40f;
    const auto labels = fingerprintLabels();

    for (int ring = 1; ring <= 4; ++ring)
    {
        std::array<float, 12> ringValues{};
        ringValues.fill(static_cast<float>(ring) / 4.0f);
        g.setColour(t.text.withAlpha(ring == 4 ? 0.14f : 0.08f));
        g.strokePath(fingerprintPath(area, ringValues),
                     juce::PathStrokeType(ring == 4 ? 1.2f : 0.8f));
    }

    g.setFont(t.monoFont().withHeight(9.4f));
    g.setColour(t.textSecondary.withAlpha(0.88f));
    for (int index = 0; index < static_cast<int>(labels.size()); ++index)
    {
        const auto outer =
            fingerprintPoint(center, radius + 16.0f, index, static_cast<int>(labels.size()), 1.0f);
        const auto axis =
            fingerprintPoint(center, radius, index, static_cast<int>(labels.size()), 1.0f);
        g.setColour(t.text.withAlpha(0.08f));
        g.drawLine(center.x, center.y, axis.x, axis.y, 1.0f);
        g.setColour(t.textSecondary.withAlpha(0.88f));
        g.drawFittedText(labels[static_cast<std::size_t>(index)],
                         juce::Rectangle<int>(static_cast<int>(outer.x - 20.0f),
                                              static_cast<int>(outer.y - 8.0f), 40, 16),
                         juce::Justification::centred, 1);
    }
}

void drawFingerprintShape(juce::Graphics& g, juce::Rectangle<float> area,
                          const std::array<float, 12>& axes, juce::Colour colour, float fillAlpha,
                          float strokeAlpha, float thickness)
{
    const auto path = fingerprintPath(area, axes);
    g.setColour(colour.withAlpha(fillAlpha));
    g.fillPath(path);
    g.setColour(colour.withAlpha(strokeAlpha));
    g.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
}

void drawMetricCard(juce::Graphics& g, juce::Rectangle<float> card, const Theme& t,
                    const juce::String& title, const juce::String& helper, float current,
                    float rangeMin, float rangeMax, float corridorLow, float corridorHigh,
                    float focus, juce::Colour colour, const juce::String& valueText)
{
    g.setColour(t.bg.withAlpha(0.42f));
    g.fillRoundedRectangle(card, 10.0f);
    g.setColour(t.text.withAlpha(0.10f));
    g.drawRoundedRectangle(card, 10.0f, 1.0f);

    auto inner = card.reduced(8.0f, 8.0f);
    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(12.0f));
    g.drawText(title, inner.removeFromTop(14).toNearestInt(), juce::Justification::centredLeft);
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(10.2f));
    g.drawFittedText(helper, inner.removeFromTop(22).toNearestInt(), juce::Justification::topLeft,
                     2);

    auto plot = inner.removeFromTop(40.0f).reduced(8.0f, 0.0f);
    const auto mapY = [&](float value)
    {
        return juce::jmap(juce::jlimit(rangeMin, rangeMax, value), rangeMin, rangeMax,
                          plot.getBottom(), plot.getY());
    };

    const float corridorTop = mapY(juce::jmax(corridorLow, corridorHigh));
    const float corridorBottom = mapY(juce::jmin(corridorLow, corridorHigh));
    const float targetY = mapY(focus);
    const float currentY = mapY(current);
    const float centerX = plot.getCentreX();
    g.setColour(t.text.withAlpha(0.12f));
    g.drawLine(centerX, plot.getY(), centerX, plot.getBottom(), 1.0f);
    auto corridor = juce::Rectangle<float>(plot.getX(), corridorTop, plot.getWidth(),
                                           juce::jmax(4.0f, corridorBottom - corridorTop));
    g.setColour(greenTarget().withAlpha(0.08f));
    g.fillRoundedRectangle(corridor, 4.0f);
    g.setColour(greenTarget().withAlpha(0.18f));
    g.drawRoundedRectangle(corridor, 4.0f, 1.0f);
    g.drawLine(plot.getX(), targetY, plot.getRight(), targetY, 1.0f);

    const float wickTop = juce::jmin(targetY, currentY);
    const float wickBottom = juce::jmax(targetY, currentY);
    g.setColour(colour.withAlpha(0.38f));
    g.drawLine(centerX, wickTop, centerX, wickBottom, 2.0f);

    auto body = juce::Rectangle<float>(centerX - plot.getWidth() * 0.16f,
                                       juce::jmin(currentY, targetY), plot.getWidth() * 0.32f,
                                       juce::jmax(4.0f, std::abs(currentY - targetY)));
    g.setColour(colour.withAlpha(0.86f));
    g.fillRoundedRectangle(body, 4.0f);

    g.setColour(t.textSecondary);
    g.setFont(t.monoFont().withHeight(10.6f));
    g.drawText(valueText, inner.removeFromTop(14).toNearestInt(), juce::Justification::centredLeft);
}

} // namespace

void MixSignatureMeter::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    auto panel = getLocalBounds().toFloat().reduced(10.0f);
    const float phase = model ? model->visualPhase : 0.0f;
    const float pulse = model ? model->visualPulse : 0.0f;
    const float shimmer = 0.5f + 0.5f * std::sin(phase * 1.25f);

    juce::ColourGradient panelGradient(t.panel.brighter(0.04f), panel.getX(), panel.getY(),
                                       t.panelAlt.darker(0.12f), panel.getX(), panel.getBottom(),
                                       false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panel, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(panel, t.cornerRadius, 1.0f);

    auto bounds = panel.reduced(16.0f, 14.0f);
    auto titleArea = bounds.removeFromTop(18);
    auto helperArea = bounds.removeFromTop(18);
    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(15.5f));
    g.drawText(panelTitle(model), titleArea.toNearestInt(), juce::Justification::left);
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(12.0f));
    g.drawFittedText(panelHelper(model), helperArea.toNearestInt(), juce::Justification::left, 2);

    bounds.removeFromTop(6.0f);
    auto fingerprintPane = bounds.removeFromLeft(220.0f);
    auto detail = bounds;

    juce::Colour accent = t.cyan;
    juce::String primaryLabel = "LIVE DNA";
    juce::String secondaryLabel = "MEASURED";
    juce::String statusLine = "Waiting for live program material.";
    juce::String fingerprintFooter = "12-axis fingerprint maps measured mix identity.";

    if (model != nullptr)
    {
        if (!model->hasLiveSignal)
        {
            primaryLabel = "LIVE DNA";
            secondaryLabel = "HOLD";
            statusLine = model->analysisStateCode == kAnalysisStateBufferWarming
                             ? "Analysis window is warming before the fingerprint unlocks."
                             : "Waiting for live program material.";
            fingerprintFooter = model->analysisStateCode == kAnalysisStateBufferWarming
                                    ? "Warming: collecting enough realtime program material for a stable fingerprint."
                                    : "Idle: fingerprint stays neutral until valid audio arrives.";
        }
        else if (model->presentationMode == UiPresentationMode::Analyze)
        {
            accent = t.cyan;
            primaryLabel = "LIVE DNA";
            secondaryLabel = "ACTIVE";
            statusLine = model->liveSessionActive
                             ? "Live fingerprint is updating from the current realtime analysis window."
                             : "Live fingerprint holds the latest stable analysis frame.";
            fingerprintFooter =
                "Analyze mode shows the live mix only. Axes reflect tone, punch, width, phase, and loudness pressure.";
        }
        else if (model->presentationMode == UiPresentationMode::Compare)
        {
            accent = t.purple;
            primaryLabel = "MIX A";
            secondaryLabel = model->compareMixB.valid ? "MIX B" : "LIVE B";
            const auto* mixA = compareMixA(model);
            statusLine = mixA != nullptr
                             ? (model->compareMixB.valid ? "Mix A and Mix B are locked for direct delta."
                                                         : "Mix A is locked; Mix B follows the live buffer.")
                             : "Capture Mix A to start direct A/B comparison.";
            fingerprintFooter =
                "Compare mode overlays two measured fingerprints. No reference pool math is used for the overlay.";
        }
        else
        {
            accent = t.purple;
            primaryLabel = "LIVE DNA";
            secondaryLabel = "TARGET";
            statusLine = model->referenceStateCode == kReferenceStateBlocked
                             ? "Reference source blocked. Live instruments remain active without target coupling."
                         : model->hasReferenceData
                             ? "Purple overlay shows the active reference corridor while values stay live."
                             : "Reference data unavailable. Live measurement mode active.";
            fingerprintFooter = model->referenceStateCode == kReferenceStateBlocked
                                    ? "Reference evaluation is blocked. The cyan fingerprint remains measurement-only."
                                : model->hasReferenceData
                                    ? "Reference mode overlays the canonical target fingerprint as a ghost polygon."
                                    : "No reference target loaded. Fingerprint remains active in neutral evaluation mode.";
        }

        if (model->presentationMode == UiPresentationMode::Analyze &&
            model->analysisStateCode == kAnalysisStateBufferWarming)
        {
            secondaryLabel = "WARM";
            statusLine = "Analysis window warming before the live fingerprint unlocks.";
            fingerprintFooter =
                "Realtime fingerprint is held back until the buffer window is trustworthy.";
        }
    }

    g.setColour(t.bg.withAlpha(0.78f));
    g.fillRoundedRectangle(fingerprintPane, 12.0f);
    g.setColour(accent.withAlpha(0.18f + 0.06f * shimmer));
    g.drawRoundedRectangle(fingerprintPane, 12.0f, 1.2f);

    auto fingerprintBounds = fingerprintPane.reduced(10.0f, 10.0f);
    auto fingerprintTagRow = fingerprintBounds.removeFromTop(18.0f);
    auto fingerprintCaption = fingerprintBounds.removeFromBottom(30.0f);
    auto fingerprintLegend = fingerprintBounds.removeFromBottom(18.0f);
    auto fingerprintArea = fingerprintBounds.reduced(0.0f, 4.0f);

    g.setColour(t.textSecondary.withAlpha(0.88f));
    g.setFont(t.monoFont().withHeight(10.8f));
    g.drawText(primaryLabel, fingerprintTagRow.removeFromLeft(92.0f).toNearestInt(),
               juce::Justification::centredLeft);
    g.drawText(secondaryLabel, fingerprintTagRow.toNearestInt(), juce::Justification::centredRight);

    if (model != nullptr && model->hasLiveSignal)
    {
        drawFingerprintGuides(g, fingerprintArea, t);

        if (model->presentationMode == UiPresentationMode::Compare)
        {
            const auto* mixA = compareMixA(model);
            const auto* mixB = compareMixB(model);
            if (mixA != nullptr)
            {
                drawFingerprintShape(g, fingerprintArea, fingerprintAxesFromMetrics(*mixA), t.purple,
                                     0.12f, 0.72f, 1.6f);
            }
            if (mixB != nullptr)
            {
                drawFingerprintShape(g, fingerprintArea, fingerprintAxesFromMetrics(*mixB), t.cyan,
                                     0.16f + 0.04f * pulse, 0.88f, 2.2f);
            }
        }
        else
        {
            bool hasTargetFingerprint = false;
            const auto liveAxes = fingerprintAxesFromMetrics(model->lastMetrics);
            const auto targetAxes = referenceFingerprintAxes(model->lastMetrics, hasTargetFingerprint);

            if (model->presentationMode == UiPresentationMode::Reference && model->hasReferenceData &&
                hasTargetFingerprint)
            {
                drawFingerprintShape(g, fingerprintArea, targetAxes, t.purple, 0.09f, 0.72f, 1.5f);
            }

            drawFingerprintShape(g, fingerprintArea, liveAxes, t.cyan, 0.16f + 0.04f * pulse, 0.92f,
                                 2.2f);

            if (model->presentationMode == UiPresentationMode::Analyze)
            {
                std::array<float, 12> envelope = liveAxes;
                for (auto& axis : envelope)
                {
                    axis = clampUnit(axis * (0.84f + 0.16f * model->signalClarity.getCurrentValue()));
                }
                drawFingerprintShape(g, fingerprintArea, envelope, t.teal, 0.05f + 0.02f * pulse,
                                     0.26f, 1.0f);
            }
        }
    }
    else
    {
        g.setColour(t.textSecondary.withAlpha(0.72f));
        g.setFont(t.bodyFont().withHeight(12.6f));
        g.drawFittedText(model != nullptr && model->analysisStateCode == kAnalysisStateBufferWarming
                             ? "Realtime fingerprint is warming."
                             : "Start playback to unlock the mix fingerprint.",
                         fingerprintArea.toNearestInt().reduced(18, 18), juce::Justification::centred,
                         2);
    }

    g.setColour(t.textSecondary.withAlpha(0.82f));
    g.setFont(t.monoFont().withHeight(10.4f));
    g.drawText(model != nullptr && model->presentationMode == UiPresentationMode::Compare
                   ? "Purple = Mix A | Cyan = Mix B/live"
                   : (model != nullptr && model->presentationMode == UiPresentationMode::Reference
                          ? "Cyan = Live | Purple = Target"
                          : "Cyan = Live fingerprint"),
               fingerprintLegend.toNearestInt(), juce::Justification::centred);
    g.setColour(t.textSecondary.withAlpha(0.78f));
    g.setFont(t.bodyFont().withHeight(11.0f));
    g.drawFittedText(fingerprintFooter, fingerprintCaption.toNearestInt(), juce::Justification::topLeft,
                     3);

    const float integratedLufs = model ? model->integratedLUFS : -99.0f;
    const float momentaryLufs = model ? model->meterMomentaryLUFS : -99.0f;
    const float truePeak = model ? model->meterTruePeakDbTP : -99.0f;
    const float peakDbFs = model ? model->meterPeakDbFS : -99.0f;
    const float widthPct = model ? model->meterStereoWidth * 100.0f : 0.0f;
    const float crest = model ? model->meterCrestFactorDb : 0.0f;
    const float transient = model ? model->transientRateHz : 0.0f;
    const float corr = model ? model->meterCorrelation : 0.0f;
    const float volatility = model ? model->meterVolatilityIndex : 0.0f;

    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(16.5f));
    g.drawText(statusLine, detail.removeFromTop(24), juce::Justification::left);

    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(13.2f));
    if (model == nullptr || !model->hasLiveSignal)
    {
        g.drawText(model != nullptr && model->analysisStateCode == kAnalysisStateBufferWarming
                       ? "Collecting enough live buffer for stable session bars and candlestick updates."
                       : "No valid program material yet. Live bars stay neutral until audio arrives.",
                   detail.removeFromTop(18), juce::Justification::left);
        g.setColour(t.textSecondary.withAlpha(0.78f));
        g.setFont(t.bodyFont().withHeight(12.4f));
        g.drawFittedText(
            "Idle state: fingerprint, cards, and band rows are intentionally withheld so stale values do not masquerade as current analysis.",
            detail.toNearestInt().reduced(0, 8), juce::Justification::topLeft, 4);
        return;
    }
    else if (model->presentationMode == UiPresentationMode::Compare)
    {
        const auto* mixA = compareMixA(model);
        const auto* mixB = compareMixB(model);
        if (mixA == nullptr || mixB == nullptr)
        {
            g.drawText("Capture Mix A, then optionally capture Mix B for a locked delta.",
                       detail.removeFromTop(18), juce::Justification::left);
        }
        else
        {
            g.drawText("Mix A LUFS " + juce::String(mixA->loudness.integratedLufs, 1) +
                           " | Mix B LUFS " + juce::String(mixB->loudness.integratedLufs, 1),
                       detail.removeFromTop(18), juce::Justification::left);
            g.drawText("Width Δ " + juce::String(mixB->stereo.width - mixA->stereo.width, 2) +
                           " | Crest Δ " +
                           juce::String(mixB->dynamics.crestFactorDb - mixA->dynamics.crestFactorDb,
                                        1) +
                           " dB",
                       detail.removeFromTop(18), juce::Justification::left);
        }
    }
    else
    {
        g.drawText("Integrated " + juce::String(integratedLufs, 1) + " LUFS | True Peak " +
                       juce::String(truePeak, 2) + " dBTP | Width " + juce::String(widthPct, 0) +
                       "%",
                   detail.removeFromTop(18), juce::Justification::left);
        g.drawText("Momentary " + juce::String(momentaryLufs, 1) + " LUFS | Peak " +
                       juce::String(peakDbFs, 2) + " dBFS | Volatility " +
                       juce::String(volatility, 2),
                   detail.removeFromTop(18), juce::Justification::left);
        g.drawText("Crest " + juce::String(crest, 1) + " dB | Correlation " +
                       juce::String(corr, 2) + " | Transients " + juce::String(transient, 1) +
                       "/s",
                   detail.removeFromTop(18), juce::Justification::left);
    }

    auto cardArea = detail.removeFromTop(110);
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(12.5f));
    g.drawText(model != nullptr && model->presentationMode == UiPresentationMode::Compare
                   ? "Delta cards"
                   : (model != nullptr && model->presentationMode == UiPresentationMode::Reference
                          ? "Reference corridor cards"
                          : "Live activity cards"),
               cardArea.removeFromTop(14), juce::Justification::left);
    auto cardGrid = cardArea.reduced(2.0f, 4.0f);
    const float cardGap = 10.0f;
    const float cardWidth = (cardGrid.getWidth() - (cardGap * 3.0f)) / 4.0f;

    if (model != nullptr && model->presentationMode == UiPresentationMode::Compare)
    {
        const auto* mixA = compareMixA(model);
        const auto* mixB = compareMixB(model);
        const float lufsDelta =
            (mixA != nullptr && mixB != nullptr) ? (mixB->loudness.integratedLufs - mixA->loudness.integratedLufs) : 0.0f;
        const float peakDelta =
            (mixA != nullptr && mixB != nullptr) ? (mixB->loudness.truePeakDbTP - mixA->loudness.truePeakDbTP) : 0.0f;
        const float widthDelta =
            (mixA != nullptr && mixB != nullptr) ? (mixB->stereo.width - mixA->stereo.width) : 0.0f;
        const float crestDelta =
            (mixA != nullptr && mixB != nullptr) ? (mixB->dynamics.crestFactorDb - mixA->dynamics.crestFactorDb) : 0.0f;

        drawMetricCard(g, {cardGrid.getX(), cardGrid.getY(), cardWidth, cardGrid.getHeight()}, t,
                       "LUFS Δ", "Mix B minus Mix A", lufsDelta, -8.0f, 8.0f, -1.0f, 1.0f, 0.0f,
                       comparisonColour(t, lufsDelta, 0.8f, 2.0f),
                       juce::String(lufsDelta >= 0.0f ? "+" : "") + juce::String(lufsDelta, 1));
        drawMetricCard(g, {cardGrid.getX() + (cardWidth + cardGap), cardGrid.getY(), cardWidth,
                           cardGrid.getHeight()},
                       t, "Peak Δ", "True peak change", peakDelta, -4.0f, 4.0f, -0.4f, 0.4f, 0.0f,
                       comparisonColour(t, peakDelta, 0.3f, 1.0f),
                       juce::String(peakDelta >= 0.0f ? "+" : "") + juce::String(peakDelta, 2));
        drawMetricCard(g, {cardGrid.getX() + 2.0f * (cardWidth + cardGap), cardGrid.getY(),
                           cardWidth, cardGrid.getHeight()},
                       t, "Width Δ", "Stereo spread change", widthDelta, -0.5f, 0.5f, -0.08f, 0.08f,
                       0.0f, comparisonColour(t, widthDelta, 0.08f, 0.22f),
                       juce::String(widthDelta >= 0.0f ? "+" : "") + juce::String(widthDelta, 2));
        drawMetricCard(g, {cardGrid.getX() + 3.0f * (cardWidth + cardGap), cardGrid.getY(),
                           cardWidth, cardGrid.getHeight()},
                       t, "Crest Δ", "Punch change", crestDelta, -8.0f, 8.0f, -1.2f, 1.2f, 0.0f,
                       comparisonColour(t, crestDelta, 1.0f, 3.0f),
                       juce::String(crestDelta >= 0.0f ? "+" : "") + juce::String(crestDelta, 1));
    }
    else
    {
        const float loudnessTarget =
            model != nullptr && model->presentationMode == UiPresentationMode::Reference &&
                    std::isfinite(model->lastMetrics.loudness.integratedReferenceLufs)
                ? model->lastMetrics.loudness.integratedReferenceLufs
                : -14.0f;
        const float peakFocus =
            model != nullptr && model->presentationMode == UiPresentationMode::Reference
                ? juce::jmax(-3.0f, model->referenceTruePeakDbTP)
                : -1.0f;
        const float widthFocus =
            model != nullptr && model->presentationMode == UiPresentationMode::Reference &&
                    model->lastMetrics.stereo.targetWidth > 0.0f
                ? model->lastMetrics.stereo.targetWidth
                : 0.32f;
        const float punchFocus =
            model != nullptr && model->presentationMode == UiPresentationMode::Reference &&
                    model->lastMetrics.dynamics.dynamicRangeReferenceDb > 0.0f
                ? model->lastMetrics.dynamics.dynamicRangeReferenceDb
                : 10.0f;
        drawMetricCard(g, {cardGrid.getX(), cardGrid.getY(), cardWidth, cardGrid.getHeight()}, t,
                       "Loudness", model != nullptr && model->presentationMode == UiPresentationMode::Reference
                                        ? "Live vs target LUFS"
                                        : "Current short term LUFS",
                       model ? model->meterShortTermLUFS : -99.0f, -28.0f, -6.0f,
                       loudnessTarget - 1.5f, loudnessTarget + 1.5f, loudnessTarget,
                       loudnessStatusColour(t, model ? model->meterShortTermLUFS : -99.0f,
                                            loudnessTarget),
                       juce::String(model ? model->meterShortTermLUFS : -99.0f, 1) + " LUFS");
        drawMetricCard(g, {cardGrid.getX() + (cardWidth + cardGap), cardGrid.getY(), cardWidth,
                           cardGrid.getHeight()},
                       t, "True Peak",
                       model != nullptr && model->presentationMode == UiPresentationMode::Reference
                           ? "Peak vs reference ceiling"
                           : "Live ceiling",
                       model ? model->meterTruePeakDbTP : -99.0f, -8.0f, 0.0f, peakFocus - 0.45f,
                       peakFocus + 0.15f, peakFocus,
                       truePeakStatusColour(t, model ? model->meterTruePeakDbTP : -99.0f),
                       juce::String(model ? model->meterTruePeakDbTP : -99.0f, 2) + " dBTP");
        drawMetricCard(g, {cardGrid.getX() + 2.0f * (cardWidth + cardGap), cardGrid.getY(),
                           cardWidth, cardGrid.getHeight()},
                       t, "Width",
                       model != nullptr && model->presentationMode == UiPresentationMode::Reference
                           ? "Target corridor"
                           : "Live stereo spread",
                       model ? model->meterStereoWidth : 0.0f, 0.0f, 1.0f, widthFocus - 0.08f,
                       widthFocus + 0.08f, widthFocus,
                       widthStatusColour(t, model ? model->meterStereoWidth : 0.0f, widthFocus),
                       juce::String(widthPct, 0) + "%");
        drawMetricCard(g, {cardGrid.getX() + 3.0f * (cardWidth + cardGap), cardGrid.getY(),
                           cardWidth, cardGrid.getHeight()},
                       t, "Punch",
                       model != nullptr && model->presentationMode == UiPresentationMode::Reference
                           ? "Crest vs target"
                           : "Crest and transient body",
                       model ? model->meterCrestFactorDb : 0.0f, 0.0f, 20.0f, punchFocus - 2.0f,
                       punchFocus + 2.0f, punchFocus,
                       punchStatusColour(t, model ? model->meterCrestFactorDb : 0.0f, punchFocus),
                       juce::String(crest, 1) + " dB");
    }

    auto tonalArea = detail.removeFromTop(120);
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(12.5f));
    g.drawText(model != nullptr && model->presentationMode == UiPresentationMode::Compare
                   ? "Band delta rows (Mix B minus Mix A)"
                   : (model != nullptr && model->presentationMode == UiPresentationMode::Reference
                          ? "Band target deviation rows"
                          : "Live band energy rows"),
               tonalArea.removeFromTop(16), juce::Justification::left);

    const std::array<juce::String, 7> names{{"Sub 20-60",   "Low 60-120",   "Low-mid 120-400",
                                             "Mid 400-2k",   "High-mid 2k-6k", "High 6k-12k",
                                             "Air 12k-20k"}};
    for (int i = 0; i < static_cast<int>(names.size()); ++i)
    {
        auto row = tonalArea.removeFromTop(14);
        auto label = row.removeFromLeft(110);
        g.setColour(t.textSecondary);
        g.drawText(names[static_cast<std::size_t>(i)], label, juce::Justification::left);

        auto bar = row.toFloat().reduced(4.0f, 2.0f);
        g.setColour(t.text.withAlpha(0.08f));
        g.fillRoundedRectangle(bar, 3.0f);

        float value = 0.0f;
        float minRange = -6.0f;
        float maxRange = 6.0f;
        juce::Colour fillColour = t.cyan.withAlpha(0.78f);
        juce::String valueText;

        if (model != nullptr && model->presentationMode == UiPresentationMode::Compare)
        {
            const auto* mixA = compareMixA(model);
            const auto* mixB = compareMixB(model);
            value = (mixA != nullptr && mixB != nullptr)
                        ? (bandValue(*mixB, i) - bandValue(*mixA, i))
                        : 0.0f;
            fillColour = comparisonColour(t, value, 1.6f, 4.5f).withAlpha(0.82f);
            valueText = juce::String(value >= 0.0f ? "+" : "") + juce::String(value, 1) + " dB";
        }
        else if (model != nullptr && model->presentationMode == UiPresentationMode::Reference)
        {
            const float rawDelta = referenceDelta(model, i);
            value = model->referenceDeltaDisplayDb[static_cast<std::size_t>(i)];
            if (std::isfinite(rawDelta))
            {
                fillColour = comparisonColour(t, value, 1.8f, 4.8f).withAlpha(0.82f);
                valueText = juce::String(value >= 0.0f ? "+" : "") + juce::String(value, 1) +
                            " dB";
            }
            else
            {
                fillColour = t.textSecondary.withAlpha(0.28f);
                valueText = "No target";
            }
        }
        else if (model != nullptr)
        {
            value = model->liveBandDisplayDb[static_cast<std::size_t>(i)];
            minRange = -42.0f;
            maxRange = 0.0f;
            fillColour = bandEnergyColour(t, value).withAlpha(0.84f);
            valueText = juce::String(value, 1) + " dB";
        }

        const float centerX = bar.getCentreX();
        const float mapped = juce::jmap(juce::jlimit(minRange, maxRange, value), minRange, maxRange,
                                        bar.getX(), bar.getRight());
        if (model != nullptr && model->presentationMode == UiPresentationMode::Analyze)
        {
            auto fill =
                juce::Rectangle<float>(bar.getX(), bar.getY(), juce::jmax(2.0f, mapped - bar.getX()),
                                       bar.getHeight());
            g.setColour(fillColour);
            g.fillRoundedRectangle(fill, 3.0f);
        }
        else
        {
            if (std::isfinite(value))
            {
                const float deltaWidth = std::abs(mapped - centerX);
                auto fill = juce::Rectangle<float>(value >= 0.0f ? centerX : centerX - deltaWidth,
                                                   bar.getY(), juce::jmax(2.0f, deltaWidth),
                                                   bar.getHeight());
                g.setColour(fillColour);
                g.fillRoundedRectangle(fill, 3.0f);
            }
            g.setColour(t.text.withAlpha(0.20f));
            g.drawLine(centerX, bar.getY(), centerX, bar.getBottom(), 1.0f);
        }

        g.setColour(t.textSecondary);
        g.setFont(t.monoFont().withHeight(10.8f));
        g.drawText(valueText, row.removeFromRight(52), juce::Justification::right);
    }

    g.setColour(t.textSecondary);
    g.setFont(t.monoFont().withHeight(11.5f));
    g.drawText(model != nullptr && model->presentationMode == UiPresentationMode::Compare
                   ? "Direct A/B mode never uses reference pool math for these band rows."
                   : (model != nullptr && model->presentationMode == UiPresentationMode::Reference
                          ? "Reference mode shows live-vs-target corridor drift from the canonical pool."
                          : "Analyze mode shows live current band energy, not reference-target placeholders."),
               detail.toNearestInt(), juce::Justification::topLeft);
}
