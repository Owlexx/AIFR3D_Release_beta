#include "ApprovalHalo.h"

#include "../Plugin/Theme.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
struct SegmentVisual
{
    juce::String label;
    float value = 0.0f;
    juce::Colour colour;
};

float clampUnit(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

juce::Colour blueUnder()
{
    return juce::Colour(0xff3b82f6);
}

juce::Colour greenTarget()
{
    return juce::Colour(0xff39ff88);
}

juce::Colour orangeRisk()
{
    return juce::Colour(0xffff9f43);
}

juce::Colour neutralInstrument()
{
    return juce::Colour(0xff8ca3b8);
}

bool usableBand(float value)
{
    return std::isfinite(value) && value > -80.0f;
}

std::array<float, 6> liveBands(const MixMetrics& metrics)
{
    return {metrics.tonal.subBandDb, metrics.tonal.lowBandDb, metrics.tonal.lowMidBandDb,
            metrics.tonal.midBandDb, metrics.tonal.highMidBandDb, metrics.tonal.highBandDb};
}

std::array<float, 6> referenceBands(const MixMetrics& metrics)
{
    return {metrics.tonal.referenceSubBandDb, metrics.tonal.referenceLowBandDb,
            metrics.tonal.referenceLowMidBandDb, metrics.tonal.referenceMidBandDb,
            metrics.tonal.referenceHighMidBandDb, metrics.tonal.referenceHighBandDb};
}

std::array<float, 6> bandDiff(const MixMetrics& left, const MixMetrics& right)
{
    const auto lhs = liveBands(left);
    const auto rhs = liveBands(right);
    return {lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2],
            lhs[3] - rhs[3], lhs[4] - rhs[4], lhs[5] - rhs[5]};
}

float bandMagnitude(float valueDb)
{
    return juce::jmap(juce::jlimit(-30.0f, 6.0f, valueDb), -30.0f, 6.0f, 0.08f, 1.0f);
}

std::array<juce::Point<float>, 6> spectrumPoints(juce::Rectangle<float> area,
                                                 const std::array<float, 6>& values)
{
    std::array<juce::Point<float>, 6> points{};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        const float x =
            juce::jmap(static_cast<float>(i), 0.0f, 5.0f, area.getX(), area.getRight());
        const float y = juce::jmap(juce::jlimit(-30.0f, 6.0f, values[i]), -30.0f, 6.0f,
                                   area.getBottom(), area.getY());
        points[i] = {x, y};
    }
    return points;
}

void drawSpectrumLine(juce::Graphics& g, const std::array<juce::Point<float>, 6>& points,
                      juce::Colour colour, float thickness)
{
    juce::Path path;
    path.startNewSubPath(points.front());
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        const auto prev = points[i - 1];
        const auto next = points[i];
        const float midX = (prev.x + next.x) * 0.5f;
        path.cubicTo({midX, prev.y}, {midX, next.y}, next);
    }

    g.setColour(colour);
    g.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
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

juce::Colour toneReferenceColour(const Theme& t, const MixMetrics& metrics)
{
    const std::array<float, 6> deltas{metrics.referenceDeltas.subBandDb, metrics.referenceDeltas.lowBandDb,
                                      metrics.referenceDeltas.lowMidBandDb, metrics.referenceDeltas.midBandDb,
                                      metrics.referenceDeltas.highMidBandDb, metrics.referenceDeltas.highBandDb};
    float dominant = 0.0f;
    bool found = false;
    for (const auto delta : deltas)
    {
        if (!std::isfinite(delta))
        {
            continue;
        }
        if (!found || std::abs(delta) > std::abs(dominant))
        {
            dominant = delta;
            found = true;
        }
    }

    if (!found)
    {
        return neutralInstrument();
    }
    if (std::abs(dominant) <= 1.0f)
    {
        return greenTarget();
    }
    if (dominant <= -3.0f)
    {
        return blueUnder();
    }
    if (dominant < 0.0f)
    {
        return t.cyan;
    }
    if (dominant >= 4.0f)
    {
        return t.redWarn;
    }
    return orangeRisk();
}

juce::Colour loudnessReferenceColour(const Theme& t, const MixMetrics& metrics)
{
    const bool hasTarget = std::isfinite(metrics.loudness.integratedReferenceLufs) &&
                           metrics.loudness.integratedReferenceLufs > -80.0f;
    if (!hasTarget)
    {
        return neutralInstrument();
    }

    const float lufsDelta = metrics.loudness.integratedLufs - metrics.loudness.integratedReferenceLufs;
    if (metrics.loudness.truePeakDbTP > -0.5f || lufsDelta > 3.0f)
    {
        return t.redWarn;
    }
    if (lufsDelta < -3.0f)
    {
        return blueUnder();
    }
    if (std::abs(lufsDelta) <= 1.2f && metrics.loudness.truePeakDbTP <= -1.0f)
    {
        return greenTarget();
    }
    if (std::abs(lufsDelta) <= 2.2f && metrics.loudness.truePeakDbTP <= -0.8f)
    {
        return t.gold;
    }
    return orangeRisk();
}

juce::Colour stereoReferenceColour(const Theme& t, const MixMetrics& metrics)
{
    const bool hasTarget = metrics.stereo.targetWidth > 0.0f || std::abs(metrics.stereo.targetCorrelation) > 0.001f;
    if (!hasTarget)
    {
        return neutralInstrument();
    }

    const float widthDelta = metrics.stereo.width - metrics.stereo.targetWidth;
    const float corrDelta = metrics.stereo.correlation - metrics.stereo.targetCorrelation;
    if (metrics.stereo.phaseRisk > 0.35f || metrics.stereo.lowBandPhaseRisk > 0.20f ||
        metrics.stereo.correlation < 0.0f)
    {
        return t.redWarn;
    }
    if (widthDelta < -0.12f)
    {
        return blueUnder();
    }
    if (std::abs(widthDelta) <= 0.06f && std::abs(corrDelta) <= 0.10f)
    {
        return greenTarget();
    }
    if (std::abs(widthDelta) <= 0.12f && std::abs(corrDelta) <= 0.18f)
    {
        return t.gold;
    }
    return orangeRisk();
}

juce::Colour dynamicsReferenceColour(const Theme& t, const MixMetrics& metrics)
{
    const float target = metrics.dynamics.dynamicRangeReferenceDb;
    if (!std::isfinite(target) || target <= 0.0f)
    {
        return neutralInstrument();
    }

    const float delta = metrics.dynamics.crestFactorDb - target;
    if (delta < -4.0f || metrics.dynamics.transientDensity < 0.18f)
    {
        return t.redWarn;
    }
    if (delta < -1.5f)
    {
        return orangeRisk();
    }
    if (std::abs(delta) <= 1.2f)
    {
        return greenTarget();
    }
    if (delta > 3.0f)
    {
        return blueUnder();
    }
    return t.gold;
}

float liveToneValue(const MixMetrics& metrics)
{
    const auto bands = liveBands(metrics);
    float sum = 0.0f;
    int count = 0;
    for (const auto band : bands)
    {
        if (usableBand(band))
        {
            sum += bandMagnitude(band);
            ++count;
        }
    }

    const float movement =
        clampUnit((std::abs(metrics.tonal.lowMidBandDb - metrics.tonal.midBandDb) +
                   std::abs(metrics.tonal.highMidBandDb - metrics.tonal.midBandDb)) /
                  18.0f);
    const float average = count > 0 ? (sum / static_cast<float>(count)) : 0.0f;
    return clampUnit(0.45f * average + 0.55f * movement);
}

float liveStereoValue(const MixMetrics& metrics)
{
    const float width = clampUnit(metrics.stereo.width);
    const float correlation = clampUnit(0.5f * (metrics.stereo.correlation + 1.0f));
    const float side = clampUnit(metrics.stereo.sideEnergy);
    return clampUnit(0.40f * width + 0.32f * side + 0.28f * correlation);
}

float liveLoudnessValue(const MixMetrics& metrics)
{
    const float lufs = clampUnit((metrics.loudness.shortTermLufs + 24.0f) / 18.0f);
    const float peak = clampUnit(1.0f - juce::jmax(0.0f, metrics.loudness.truePeakDbTP + 1.0f) / 3.0f);
    return clampUnit(0.60f * lufs + 0.40f * peak);
}

float liveDynamicsValue(const MixMetrics& metrics)
{
    const float crest = clampUnit((metrics.dynamics.crestFactorDb - 3.0f) / 12.0f);
    const float transient = clampUnit(metrics.dynamics.transientDensity);
    return clampUnit(0.56f * crest + 0.44f * transient);
}

juce::Colour liveToneColour(const Theme& t, const MixMetrics& metrics)
{
    const float tilt = metrics.tonal.spectralBalance;
    if (metrics.flags & kFlagHarshnessBurst)
    {
        return t.redWarn;
    }
    if (tilt <= -4.0f)
    {
        return blueUnder();
    }
    if (tilt <= -1.5f)
    {
        return t.cyan;
    }
    if (tilt <= 2.5f)
    {
        return greenTarget();
    }
    if (tilt <= 5.0f)
    {
        return t.gold;
    }
    return t.redWarn;
}

juce::Colour liveStereoColour(const Theme& t, const MixMetrics& metrics)
{
    if (metrics.stereo.phaseRisk > 0.35f || metrics.stereo.correlation < 0.0f)
    {
        return t.redWarn;
    }
    if (metrics.stereo.width < 0.14f)
    {
        return blueUnder();
    }
    if (metrics.stereo.width <= 0.22f)
    {
        return t.cyan;
    }
    if (metrics.stereo.width <= 0.58f && metrics.stereo.correlation >= 0.10f)
    {
        return greenTarget();
    }
    if (metrics.stereo.width <= 0.72f)
    {
        return t.gold;
    }
    return t.redWarn;
}

juce::Colour liveLoudnessColour(const Theme& t, const MixMetrics& metrics)
{
    if (metrics.loudness.truePeakDbTP > -0.35f || metrics.loudness.shortTermLufs > -7.5f)
    {
        return t.redWarn;
    }
    if (metrics.loudness.shortTermLufs < -20.0f)
    {
        return blueUnder();
    }
    if (metrics.loudness.shortTermLufs < -16.0f)
    {
        return t.cyan;
    }
    if (metrics.loudness.shortTermLufs <= -11.0f && metrics.loudness.truePeakDbTP <= -1.0f)
    {
        return greenTarget();
    }
    if (metrics.loudness.shortTermLufs <= -9.0f && metrics.loudness.truePeakDbTP <= -0.7f)
    {
        return t.gold;
    }
    return orangeRisk();
}

juce::Colour liveDynamicsColour(const Theme& t, const MixMetrics& metrics)
{
    if (metrics.dynamics.crestFactorDb < 4.0f && metrics.dynamics.transientDensity < 0.15f)
    {
        return t.redWarn;
    }
    if (metrics.dynamics.crestFactorDb < 6.0f)
    {
        return blueUnder();
    }
    if (metrics.dynamics.crestFactorDb < 8.0f)
    {
        return t.cyan;
    }
    if (metrics.dynamics.crestFactorDb <= 14.5f)
    {
        return greenTarget();
    }
    if (metrics.dynamics.crestFactorDb <= 17.0f)
    {
        return t.gold;
    }
    return orangeRisk();
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

std::array<SegmentVisual, 4> buildSegments(const Theme& t, const UiModel* model)
{
    const juce::Colour neutral = neutralInstrument().withAlpha(0.78f);
    if (model == nullptr || !model->hasLiveSignal)
    {
        return {{{"TONE", 0.0f, neutral},
                 {"WIDTH", 0.0f, neutral},
                 {"PEAK", 0.0f, neutral},
                 {"PUNCH", 0.0f, neutral}}};
    }

    if (model->presentationMode == UiPresentationMode::Compare)
    {
        const auto* mixA = compareMixA(model);
        const auto* mixB = compareMixB(model);
        if (mixA == nullptr || mixB == nullptr)
        {
            return {{{"TONE Δ", 0.0f, neutral},
                     {"WIDTH Δ", 0.0f, neutral},
                     {"PEAK Δ", 0.0f, neutral},
                     {"PUNCH Δ", 0.0f, neutral}}};
        }

        const auto deltas = bandDiff(*mixB, *mixA);
        const float toneDelta =
            (std::abs(deltas[0]) + std::abs(deltas[1]) + std::abs(deltas[2]) +
             std::abs(deltas[3]) + std::abs(deltas[4]) + std::abs(deltas[5])) /
            6.0f;
        const float widthDelta = mixB->stereo.width - mixA->stereo.width;
        const float peakDelta = mixB->loudness.truePeakDbTP - mixA->loudness.truePeakDbTP;
        const float punchDelta = mixB->dynamics.crestFactorDb - mixA->dynamics.crestFactorDb;

        return {{{"TONE Δ", clampUnit(std::abs(toneDelta) / 6.0f),
                  comparisonColour(t, toneDelta, 1.5f, 4.0f)},
                 {"WIDTH Δ", clampUnit(std::abs(widthDelta) / 0.35f),
                  comparisonColour(t, widthDelta, 0.05f, 0.18f)},
                 {"PEAK Δ", clampUnit(std::abs(peakDelta) / 3.0f),
                  comparisonColour(t, peakDelta, 0.4f, 1.2f)},
                 {"PUNCH Δ", clampUnit(std::abs(punchDelta) / 4.0f),
                  comparisonColour(t, punchDelta, 0.8f, 2.5f)}}};
    }

    const auto& metrics = model->lastMetrics;
    if (model->presentationMode == UiPresentationMode::Reference)
    {
        const juce::Colour tone = model->hasReferenceData ? toneReferenceColour(t, metrics) : neutral;
        const juce::Colour stereo =
            model->hasReferenceData ? stereoReferenceColour(t, metrics) : neutral;
        const juce::Colour loud =
            model->hasReferenceData ? loudnessReferenceColour(t, metrics) : neutral;
        const juce::Colour dynamics =
            model->hasReferenceData ? dynamicsReferenceColour(t, metrics) : neutral;
        return {{{"TONE", liveToneValue(metrics), tone},
                 {"STEREO", liveStereoValue(metrics), stereo},
                 {"LOUD", liveLoudnessValue(metrics), loud},
                 {"DYNAMICS", liveDynamicsValue(metrics), dynamics}}};
    }

    return {{{"TONE", model->liveToneMeter, liveToneColour(t, metrics)},
             {"WIDTH", model->liveStereoMeter, liveStereoColour(t, metrics)},
             {"PEAK", model->liveLoudnessMeter, liveLoudnessColour(t, metrics)},
             {"PUNCH", model->liveDynamicsMeter, liveDynamicsColour(t, metrics)}}};
}

void drawSegmentArc(juce::Graphics& g, juce::Rectangle<float> area, float startAngle,
                    float endAngle, float value, juce::Colour colour, float thickness)
{
    juce::Path path;
    const auto ring = area.reduced(thickness * 0.5f);
    path.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f,
                       ring.getHeight() * 0.5f, 0.0f, startAngle,
                       lerp(startAngle, endAngle, value), true);
    g.setColour(colour);
    g.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
}

void drawSpectrumLayer(juce::Graphics& g, juce::Rectangle<float> area, const Theme& t,
                       const UiModel* model)
{
    if (model == nullptr || !model->hasValidSpectralData)
    {
        return;
    }

    g.setColour(t.text.withAlpha(0.04f));
    for (int i = 0; i < 4; ++i)
    {
        const float y = juce::jmap(static_cast<float>(i), 0.0f, 3.0f, area.getY(), area.getBottom());
        g.drawLine(area.getX(), y, area.getRight(), y, 1.0f);
    }

    std::array<float, 6> liveValues = {model->liveBandDisplayDb[0], model->liveBandDisplayDb[1],
                                       model->liveBandDisplayDb[2], model->liveBandDisplayDb[3],
                                       model->liveBandDisplayDb[4], model->liveBandDisplayDb[5]};
    std::array<float, 6> compareValues{};
    bool drawCompareOverlay = false;
    juce::Colour overlayColour = t.purple.withAlpha(0.42f);

    if (model->presentationMode == UiPresentationMode::Compare)
    {
        if (const auto* mixA = compareMixA(model); mixA != nullptr)
        {
            compareValues = liveBands(*mixA);
            drawCompareOverlay = true;
        }
    }
    else if (model->presentationMode == UiPresentationMode::Reference)
    {
        compareValues = referenceBands(model->lastMetrics);
        drawCompareOverlay = model->hasReferenceData;
    }

    const float sideGlow = clampUnit(model->sideEnergy.getCurrentValue());
    if (drawCompareOverlay)
    {
        drawSpectrumLine(g, spectrumPoints(area, compareValues), overlayColour, 1.8f);
    }

    const auto points = spectrumPoints(area, liveValues);
    drawSpectrumLine(g, points, t.cyan.withAlpha(0.18f + 0.32f * sideGlow), 5.8f);
    drawSpectrumLine(g, points, t.cyan.withAlpha(0.82f), 2.35f);
}

juce::String centerPrimaryText(const UiModel* model)
{
    if (model == nullptr)
    {
        return "IDLE";
    }

    if (!model->hasLiveSignal)
    {
        return "IDLE";
    }

    if (model->presentationMode == UiPresentationMode::Analyze)
    {
        if (model->analysisStateCode == kAnalysisStateBufferWarming)
        {
            return "WARM";
        }
        if (model->analysisStateCode == kAnalysisStateActiveLive && !model->hasScoredResult)
        {
            return "LIVE";
        }
        return juce::String(model->meterShortTermLUFS, 1) + " LUFS";
    }

    if (model->presentationMode == UiPresentationMode::Compare)
    {
        const auto* mixA = compareMixA(model);
        const auto* mixB = compareMixB(model);
        if (mixA == nullptr || mixB == nullptr)
        {
            return "CAPTURE";
        }

        const auto deltas = bandDiff(*mixB, *mixA);
        const float toneDelta =
            (std::abs(deltas[0]) + std::abs(deltas[1]) + std::abs(deltas[2]) +
             std::abs(deltas[3]) + std::abs(deltas[4]) + std::abs(deltas[5])) /
            6.0f;
        const float similarity =
            1.0f - clampUnit(0.45f * (toneDelta / 6.0f) +
                             0.20f * (std::abs(mixB->stereo.width - mixA->stereo.width) / 0.35f) +
                             0.15f * (std::abs(mixB->loudness.truePeakDbTP -
                                               mixA->loudness.truePeakDbTP) /
                                      3.0f) +
                             0.20f * (std::abs(mixB->dynamics.crestFactorDb -
                                               mixA->dynamics.crestFactorDb) /
                                      4.0f));
        return juce::String(juce::roundToInt(similarity * 100.0f)) + "%";
    }

    if (!model->hasReferenceData)
    {
        return "LIVE";
    }
    if (!model->hasScoredResult)
    {
        return model->analysisStateCode == kAnalysisStateBufferWarming ? "WARM" : "ALIGN";
    }
    return juce::String(juce::roundToInt(model->mixAlignment.getCurrentValue() * 100.0f)) + "%";
}

juce::String centerSecondaryText(const UiModel* model)
{
    if (model == nullptr)
    {
        return "No signal";
    }

    if (model->presentationMode == UiPresentationMode::Reference && !model->hasReferenceData)
    {
        return model->referenceStateCode == kReferenceStateBlocked ? "Reference blocked"
                                                                   : "Live measurement mode";
    }

    if (!model->hasLiveSignal)
    {
        return "No signal";
    }

    if (model->presentationMode == UiPresentationMode::Analyze)
    {
        if (model->analysisStateCode == kAnalysisStateBufferWarming)
        {
            return "Collecting analysis window";
        }
        if (model->analysisStateCode == kAnalysisStateActiveLive && !model->hasScoredResult)
        {
            return "Live buffer, reference context warming";
        }
        return "TP " + juce::String(model->meterTruePeakDbTP, 2) + " dBTP";
    }
    if (model->presentationMode == UiPresentationMode::Compare)
    {
        if (!model->compareMixA.valid)
        {
            return "Capture Mix A";
        }
        if (model->compareMixB.valid)
        {
            return model->compareMixA.label + " vs " + model->compareMixB.label;
        }
        return model->compareMixA.label + " vs Live B";
    }
    if (!model->hasReferenceData)
    {
        return "Live values, neutral evaluation";
    }
    if (!model->hasScoredResult)
    {
        return "Reference context aligning";
    }
    return "Signal clarity " +
           juce::String(juce::roundToInt(model->signalClarity.getCurrentValue() * 100.0f)) + "%";
}

juce::String modeTitle(const UiModel* model)
{
    if (model == nullptr)
    {
        return "LIVE DIAGNOSTIC HALO";
    }

    switch (model->presentationMode)
    {
    case UiPresentationMode::Compare:
        return "A/B DELTA HALO";
    case UiPresentationMode::Reference:
        return "REFERENCE ALIGNMENT HALO";
    case UiPresentationMode::Analyze:
    default:
        return "LIVE DIAGNOSTIC HALO";
    }
}

juce::String footerLine(const UiModel* model)
{
    if (model == nullptr)
    {
        return "Blue under | Cyan close | Green target | Gold risk | Red problem";
    }

    switch (model->presentationMode)
    {
    case UiPresentationMode::Compare:
        return "Blue/cyan = Mix B lower than Mix A | Green = stable | Gold/red = Mix B hotter or wider";
    case UiPresentationMode::Reference:
        if (!model->hasReferenceData)
        {
            return "Reference unavailable: halo stays live in neutral instrument colors";
        }
        return "Arc length shows live signal. Arc color shows reference corridor evaluation.";
    case UiPresentationMode::Analyze:
    default:
        return "Segments show live tone, width, peak, and punch from the realtime buffer";
    }
}

} // namespace

void ApprovalHalo::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    const auto segments = buildSegments(t, model);
    auto panel = getLocalBounds().toFloat().reduced(10.0f);
    auto layout = getLocalBounds().reduced(20, 14);
    auto headerArea = layout.removeFromTop(26);
    auto colorLegendArea = layout.removeFromBottom(16);
    auto legendArea = layout.removeFromBottom(18);
    auto issueArea = layout.removeFromBottom(34);

    const float pulse = model ? model->visualPulse : 0.0f;
    const bool hasLiveSignal = model != nullptr && model->hasLiveSignal;
    const bool hasReference = model != nullptr && model->hasReferenceData;

    juce::ColourGradient panelGradient(t.panel.brighter(0.05f), panel.getX(), panel.getY(),
                                       t.panelAlt.darker(0.12f), panel.getX(), panel.getBottom(),
                                       false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panel, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(panel, t.cornerRadius, 1.0f);

    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(15.5f));
    g.drawFittedText(modeTitle(model), headerArea.removeFromLeft(headerArea.getWidth() - 180),
                     juce::Justification::centredLeft, 1);

    const auto pulseLabel = pulseMode == PulseMode::Dynamics   ? "Pulse focus: Dynamics"
                            : pulseMode == PulseMode::Stereo   ? "Pulse focus: Stereo"
                                                               : "Pulse focus: Loudness";
    g.setColour(t.textSecondary.withAlpha(0.78f));
    g.setFont(t.monoFont().withHeight(10.8f));
    g.drawFittedText(pulseLabel, headerArea, juce::Justification::centredRight, 1);

    auto ringArea = layout.toFloat().reduced(8.0f, 4.0f);
    ringArea = ringArea.withSizeKeepingCentre(juce::jmin(ringArea.getWidth(), ringArea.getHeight()),
                                              juce::jmin(ringArea.getWidth(), ringArea.getHeight()));

    g.setColour(t.text.withAlpha(0.06f));
    g.drawEllipse(ringArea, 10.0f);

    auto analyzerArea =
        ringArea.reduced(ringArea.getWidth() * 0.19f, ringArea.getHeight() * 0.25f);
    drawSpectrumLayer(g, analyzerArea, t, model);

    const float thickness = 12.0f + 1.5f * pulse;
    const float start = -juce::MathConstants<float>::pi * 0.75f;
    const float segmentAngle = juce::MathConstants<float>::halfPi;
    const float gap = segmentAngle * 0.12f;

    const auto pulseArea = ringArea.expanded(10.0f + 11.0f * pulse);
    const float pulseThickness = 4.2f + 2.8f * pulse;
    for (int i = 0; i < 4; ++i)
    {
        const float segmentStart = start + static_cast<float>(i) * segmentAngle + gap * 0.5f;
        const float segmentEnd =
            start + static_cast<float>(i + 1) * segmentAngle - gap * 0.5f;
        const float pulseScale = i == 1 ? (pulseMode == PulseMode::Stereo ? 1.0f : 0.72f)
                              : i == 2 ? (pulseMode == PulseMode::Loudness ? 1.0f : 0.72f)
                              : i == 3 ? (pulseMode == PulseMode::Dynamics ? 1.0f : 0.72f)
                                       : 0.84f;
        const float pulseValue =
            hasLiveSignal
                ? clampUnit(0.18f + 0.82f * segments[static_cast<std::size_t>(i)].value * pulseScale)
                : 0.05f;
        drawSegment(g, pulseArea, segmentStart, segmentEnd, pulseValue,
                    segments[static_cast<std::size_t>(i)].colour.withAlpha(0.22f + 0.18f * pulse),
                    pulseThickness + 2.0f);
        drawSegment(g, ringArea, segmentStart, segmentEnd,
                    segments[static_cast<std::size_t>(i)].value,
                    segments[static_cast<std::size_t>(i)].colour, thickness);
    }

    const float centerRadius = ringArea.getWidth() * 0.27f;
    juce::Colour statusColour =
        hasLiveSignal ? segments[0].colour.interpolatedWith(segments[1].colour, 0.5f)
                      : t.textSecondary.withAlpha(0.55f);
    if (model != nullptr && model->presentationMode == UiPresentationMode::Reference && hasReference)
    {
        statusColour = t.purple.interpolatedWith(statusColour, 0.45f);
    }
    juce::ColourGradient coreGlow(statusColour.withAlpha(0.30f + 0.12f * pulse),
                                  ringArea.getCentreX(), ringArea.getCentreY(),
                                  t.bg.withAlpha(0.04f), ringArea.getX(), ringArea.getY(), true);
    g.setGradientFill(coreGlow);
    g.fillEllipse(ringArea.getCentreX() - centerRadius, ringArea.getCentreY() - centerRadius,
                  centerRadius * 2.0f, centerRadius * 2.0f);

    const juce::String headerLabel = model != nullptr ? model->haloLabel : juce::String("Listening");
    const juce::String detailText =
        model != nullptr ? model->haloDetail : juce::String("Waiting for live buffer");
    g.setColour(statusColour.withAlpha(0.95f));
    g.setFont(t.bodyFont().boldened().withHeight(14.2f));
    g.drawText(headerLabel, ringArea.toNearestInt().translated(0, -46), juce::Justification::centred);

    g.setColour(t.text);
    g.setFont(t.titleFont().withHeight(20.0f));
    g.drawText(centerPrimaryText(model), ringArea.toNearestInt().translated(0, -2),
               juce::Justification::centred);

    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(12.8f));
    g.drawText(centerSecondaryText(model), ringArea.toNearestInt().translated(0, 24),
               juce::Justification::centred);
    g.drawFittedText(detailText, ringArea.toNearestInt().translated(0, 44), juce::Justification::centred,
                     2);

    g.setColour(t.textSecondary.withAlpha(0.92f));
    g.setFont(t.monoFont().withHeight(11.2f));
    g.drawFittedText(segments[0].label,
                     juce::Rectangle<int>(ringArea.getCentreX() - 46, ringArea.getY() - 20, 92, 14),
                     juce::Justification::centred, 1);
    g.drawFittedText(segments[1].label,
                     juce::Rectangle<int>(ringArea.getRight() + 6, ringArea.getCentreY() - 8, 86, 14),
                     juce::Justification::centredLeft, 1);
    g.drawFittedText(segments[2].label,
                     juce::Rectangle<int>(ringArea.getCentreX() - 58, ringArea.getBottom() + 4, 116, 14),
                     juce::Justification::centred, 1);
    g.drawFittedText(segments[3].label,
                     juce::Rectangle<int>(ringArea.getX() - 84, ringArea.getCentreY() - 8, 82, 14),
                     juce::Justification::centredRight, 1);

    juce::String issueText = "Waiting for live buffer";
    if (model != nullptr)
    {
        issueText = model->presentationMode == UiPresentationMode::Compare
                        ? centerSecondaryText(model)
                        : (model->hasLiveSignal ? model->tonalSummary : model->statusText);
    }
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(13.2f));
    g.drawFittedText(issueText, issueArea, juce::Justification::centred, 2);

    g.setFont(t.monoFont().withHeight(11.8f));
    g.drawFittedText(footerLine(model), legendArea, juce::Justification::centred, 1);

    const std::array<std::pair<juce::Colour, juce::String>, 5> legend{{
        {blueUnder(), "Under"},
        {t.cyan, "Close"},
        {greenTarget(), "Target"},
        {t.gold, "Risk"},
        {t.redWarn, "Problem"},
    }};
    int x = colorLegendArea.getX() + 18;
    for (const auto& entry : legend)
    {
        g.setColour(entry.first);
        g.fillEllipse(static_cast<float>(x), static_cast<float>(colorLegendArea.getY() + 4), 8.0f,
                      8.0f);
        x += 12;
        g.setColour(t.textSecondary);
        g.setFont(t.monoFont().withHeight(10.4f));
        g.drawText(entry.second, x, colorLegendArea.getY(), 52, colorLegendArea.getHeight(),
                   juce::Justification::left);
        x += 56;
    }
}

void ApprovalHalo::drawSegment(juce::Graphics& g, juce::Rectangle<float> area, float startAngle,
                               float endAngle, float value, juce::Colour colour, float thickness)
{
    drawSegmentArc(g, area, startAngle, endAngle, value, colour, thickness);
}
