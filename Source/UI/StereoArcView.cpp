#include "StereoArcView.h"

#include "../Plugin/Theme.h"

#include <cmath>

namespace
{
float clampUnit(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float correlationNorm(float correlation)
{
    return clampUnit(0.5f * (correlation + 1.0f));
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
        return "SPATIAL FIELD";
    }

    switch (model->presentationMode)
    {
    case UiPresentationMode::Compare:
        return "SPATIAL FIELD (COMPARE)";
    case UiPresentationMode::Reference:
        return "SPATIAL FIELD (REFERENCE)";
    case UiPresentationMode::Analyze:
    default:
        return "SPATIAL FIELD (LIVE)";
    }
}

juce::String helperText(const UiModel* model)
{
    if (model == nullptr)
    {
        return "Measured WXYZ field is idle.";
    }

    switch (model->presentationMode)
    {
    case UiPresentationMode::Compare:
        return "Purple = Mix A. Cyan = Mix B/live. Polygons come from measured WXYZ spatial energy only.";
    case UiPresentationMode::Reference:
        return "Cyan = live field. Purple = canonical reference width/correlation target.";
    case UiPresentationMode::Analyze:
    default:
        return "W = mid, X = left, Y = right, Z = side. Correlation and low-end mono risk are measured below.";
    }
}

struct SpatialShape
{
    juce::Path field;
    juce::Rectangle<float> envelope;
    juce::Point<float> w;
    juce::Point<float> x;
    juce::Point<float> y;
    juce::Point<float> z;
    juce::Point<float> center;
};

SpatialShape buildSpatialShape(juce::Rectangle<float> area, const StereoMetrics& metrics)
{
    SpatialShape shape;
    const float spread = juce::jmap(metrics.spatialSpread, 0.0f, 1.0f, 0.62f, 1.18f);
    const float corr = correlationNorm(metrics.correlation);
    const float width = clampUnit(metrics.width);
    const float phase = clampUnit(metrics.phaseRisk);
    const float xBias = juce::jlimit(-1.0f, 1.0f, metrics.spatialY - metrics.spatialX) *
                        area.getWidth() * 0.12f;

    shape.center = area.getCentre() + juce::Point<float>(xBias, 0.0f);

    const float xRadius = area.getWidth() * juce::jmap(width, 0.0f, 1.0f, 0.16f, 0.40f) * spread;
    const float yRadius =
        area.getHeight() * juce::jmap(metrics.centerDominance, 0.0f, 1.0f, 0.16f, 0.34f);
    const float zRadius =
        area.getHeight() * juce::jmap(metrics.sideDominance, 0.0f, 1.0f, 0.10f, 0.34f);
    const float axisBend = area.getHeight() * 0.08f * phase;

    const float wValue = clampUnit(metrics.spatialW);
    const float xValue = clampUnit(metrics.spatialX);
    const float yValue = clampUnit(metrics.spatialY);
    const float zValue = clampUnit(metrics.spatialZ);

    shape.w = {shape.center.x,
               shape.center.y - yRadius * juce::jmap(wValue, 0.0f, 1.0f, 0.10f, 1.05f)};
    shape.x = {shape.center.x - xRadius * juce::jmap(xValue, 0.0f, 1.0f, 0.12f, 1.0f),
               shape.center.y - axisBend * (1.0f - corr) - yRadius * 0.10f * phase};
    shape.y = {shape.center.x + xRadius * juce::jmap(yValue, 0.0f, 1.0f, 0.12f, 1.0f),
               shape.center.y - axisBend * (1.0f - corr) - yRadius * 0.10f * phase};
    shape.z = {shape.center.x,
               shape.center.y + zRadius * juce::jmap(zValue, 0.0f, 1.0f, 0.12f, 1.05f) +
                   axisBend * 0.55f};

    shape.field.startNewSubPath(shape.w);
    shape.field.quadraticTo({shape.center.x + xRadius * (0.38f + 0.18f * width),
                             shape.center.y - yRadius * (0.10f + 0.26f * corr)},
                            shape.y);
    shape.field.quadraticTo({shape.center.x + xRadius * (0.24f + 0.28f * width),
                             shape.center.y + zRadius * (0.34f + 0.28f * phase)},
                            shape.z);
    shape.field.quadraticTo({shape.center.x - xRadius * (0.24f + 0.28f * width),
                             shape.center.y + zRadius * (0.34f + 0.28f * phase)},
                            shape.x);
    shape.field.quadraticTo({shape.center.x - xRadius * (0.38f + 0.18f * width),
                             shape.center.y - yRadius * (0.10f + 0.26f * corr)},
                            shape.w);
    shape.field.closeSubPath();

    const float envelopeWidth =
        area.getWidth() * juce::jmap(width, 0.0f, 1.0f, 0.12f, 0.82f) * spread;
    const float envelopeHeight =
        area.getHeight() * juce::jmap(corr, 0.0f, 1.0f, 0.22f, 0.62f) * (1.0f + phase * 0.18f);
    shape.envelope =
        juce::Rectangle<float>(shape.center.x - envelopeWidth * 0.5f,
                               shape.center.y - envelopeHeight * 0.5f, envelopeWidth,
                               envelopeHeight);
    return shape;
}

StereoMetrics referenceOverlayMetrics(const MixMetrics& metrics)
{
    StereoMetrics overlay = metrics.stereo;
    const float targetWidth =
        overlay.referenceWidth > 0.0f ? overlay.referenceWidth : overlay.targetWidth;
    const float targetCorrelation =
        std::abs(overlay.referenceCorrelation) > 0.001f ? overlay.referenceCorrelation
                                                        : overlay.targetCorrelation;

    overlay.width = clampUnit(targetWidth);
    overlay.correlation = juce::jlimit(-1.0f, 1.0f, targetCorrelation);
    overlay.sideEnergy = overlay.width;
    overlay.midEnergy = clampUnit(1.0f - overlay.width * 0.58f);
    overlay.leftEnergy = 0.5f;
    overlay.rightEnergy = 0.5f;
    overlay.centerDominance = clampUnit(1.0f - overlay.width * 0.50f);
    overlay.sideDominance = overlay.width;
    overlay.stereoMotion = 0.0f;
    overlay.spatialSpread = clampUnit(0.35f + overlay.width * 0.55f);
    overlay.lowBandCorrelation = overlay.correlation;
    overlay.lowBandPhaseRisk = clampUnit((0.2f - overlay.lowBandCorrelation) / 0.6f);
    overlay.subMonoIntegrity = clampUnit(0.5f * (overlay.lowBandCorrelation + 1.0f));
    overlay.phaseRisk = clampUnit((0.25f - overlay.correlation) / 0.8f);
    overlay.spatialW = overlay.midEnergy;
    overlay.spatialX = overlay.leftEnergy;
    overlay.spatialY = overlay.rightEnergy;
    overlay.spatialZ = overlay.sideEnergy;
    return overlay;
}

juce::String spatialStateText(const StereoMetrics& metrics)
{
    if (metrics.lowBandPhaseRisk > 0.20f)
    {
        return "LOW-END WIDTH RISK";
    }
    if (metrics.correlation < 0.0f || metrics.phaseRisk > 0.35f)
    {
        return "PHASE DANGER";
    }
    if (metrics.width < 0.12f)
    {
        return "MONO COLLAPSE";
    }
    if (metrics.width < 0.30f)
    {
        return "CENTERED STEREO";
    }
    if (metrics.width > 0.78f)
    {
        return "EXTREME SIDE";
    }
    return "HEALTHY STEREO";
}

juce::Colour spatialStateColour(const Theme& t, const StereoMetrics& metrics)
{
    if (metrics.lowBandPhaseRisk > 0.20f)
    {
        return t.redWarn;
    }
    if (metrics.correlation < 0.0f || metrics.phaseRisk > 0.35f)
    {
        return t.redWarn;
    }
    if (metrics.width < 0.12f)
    {
        return t.gold;
    }
    if (metrics.width < 0.30f)
    {
        return t.cyan;
    }
    if (metrics.width > 0.78f)
    {
        return t.gold;
    }
    return juce::Colour(0xff39ff88);
}

void drawAxisGuides(juce::Graphics& g, juce::Rectangle<float> area, const Theme& t,
                    const SpatialShape& shape)
{
    g.setColour(t.text.withAlpha(0.08f));
    g.drawLine(shape.center.x, area.getY(), shape.center.x, area.getBottom(), 1.0f);
    g.drawLine(area.getX(), shape.center.y, area.getRight(), shape.center.y, 1.0f);
    g.drawEllipse(shape.envelope, 1.0f);

    g.setColour(t.textSecondary.withAlpha(0.80f));
    g.setFont(t.monoFont().withHeight(10.8f));
    g.drawText("W", juce::Rectangle<int>(static_cast<int>(shape.center.x - 10.0f),
                                         static_cast<int>(area.getY()), 20, 12),
               juce::Justification::centred);
    g.drawText("X", juce::Rectangle<int>(static_cast<int>(area.getX()),
                                         static_cast<int>(shape.center.y - 8.0f), 18, 16),
               juce::Justification::centredLeft);
    g.drawText("Y", juce::Rectangle<int>(static_cast<int>(area.getRight() - 18.0f),
                                         static_cast<int>(shape.center.y - 8.0f), 18, 16),
               juce::Justification::centredRight);
    g.drawText("Z", juce::Rectangle<int>(static_cast<int>(shape.center.x - 10.0f),
                                         static_cast<int>(area.getBottom() - 12.0f), 20, 12),
               juce::Justification::centred);
}

void drawSpatialField(juce::Graphics& g, juce::Rectangle<float> area, const Theme& t,
                      const StereoMetrics& metrics, juce::Colour colour, float alpha,
                      float thickness)
{
    const auto shape = buildSpatialShape(area, metrics);
    const float sideGlow = clampUnit(metrics.sideEnergy);
    const float motionGlow = clampUnit(metrics.stereoMotion);

    g.setColour(colour.withAlpha(alpha * (0.10f + 0.18f * sideGlow)));
    g.fillEllipse(shape.envelope.expanded(3.0f, 2.0f));
    g.setColour(colour.withAlpha(alpha * (0.20f + 0.20f * motionGlow)));
    g.fillPath(shape.field);
    g.setColour(colour.withAlpha(alpha));
    g.strokePath(shape.field,
                 juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));

    const float pointRadius = 2.8f;
    g.setColour(colour.withAlpha(alpha * 0.90f));
    g.fillEllipse(shape.w.x - pointRadius, shape.w.y - pointRadius, pointRadius * 2.0f,
                  pointRadius * 2.0f);
    g.fillEllipse(shape.x.x - pointRadius, shape.x.y - pointRadius, pointRadius * 2.0f,
                  pointRadius * 2.0f);
    g.fillEllipse(shape.y.x - pointRadius, shape.y.y - pointRadius, pointRadius * 2.0f,
                  pointRadius * 2.0f);
    g.fillEllipse(shape.z.x - pointRadius, shape.z.y - pointRadius, pointRadius * 2.0f,
                  pointRadius * 2.0f);

    drawAxisGuides(g, area, t, shape);
}

juce::String stereoFooter(const MixMetrics& metrics)
{
    return "W " + juce::String(metrics.stereo.spatialW, 2) + " | X " +
           juce::String(metrics.stereo.spatialX, 2) + " | Y " +
           juce::String(metrics.stereo.spatialY, 2) + " | Z " +
           juce::String(metrics.stereo.spatialZ, 2) + " | Corr " +
           juce::String(metrics.stereo.correlation, 2) + " | Low mono " +
           juce::String(metrics.stereo.subMonoIntegrity, 2);
}

} // namespace

void StereoArcView::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    auto panel = getLocalBounds().toFloat().reduced(10.0f);
    const float pulse = model ? model->visualPulse : 0.0f;

    juce::ColourGradient panelGradient(t.panel.brighter(0.04f), panel.getX(), panel.getY(),
                                       t.panelAlt.darker(0.13f), panel.getX(), panel.getBottom(),
                                       false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panel, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(panel, t.cornerRadius, 1.0f);

    auto bounds = panel.reduced(18.0f, 14.0f);
    auto titleArea = bounds.removeFromTop(20);
    auto helperArea = bounds.removeFromTop(16);
    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(15.5f));
    g.drawText(panelTitle(model), titleArea.toNearestInt(), juce::Justification::left);
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(12.0f));
    g.drawFittedText(helperText(model), helperArea.toNearestInt(), juce::Justification::left, 2);

    auto fieldArea = bounds.reduced(8.0f, 10.0f);
    auto footer = panel.toNearestInt().removeFromBottom(20);

    if (model == nullptr || !model->hasLiveSignal)
    {
        g.setColour(t.textSecondary.withAlpha(0.72f));
        g.setFont(t.bodyFont().withHeight(13.0f));
        g.drawFittedText(model != nullptr && model->analysisStateCode == kAnalysisStateBufferWarming
                             ? "Spatial field is warming. WXYZ metrics will lock after enough realtime program material."
                             : "No valid program material for spatial analysis yet.",
                         fieldArea.toNearestInt().reduced(12, 12), juce::Justification::centred,
                         2);
        return;
    }

    const auto& liveMetrics = model->lastMetrics;

    if (model->presentationMode == UiPresentationMode::Compare)
    {
        const auto* mixA = compareMixA(model);
        const auto* mixB = compareMixB(model);
        if (mixA != nullptr)
        {
            drawSpatialField(g, fieldArea, t, mixA->stereo, t.purple, 0.80f, 1.5f);
        }
        if (mixB != nullptr)
        {
            drawSpatialField(g, fieldArea, t, mixB->stereo, t.cyan, 0.88f + 0.10f * pulse, 2.5f);
        }

        g.setColour(t.textSecondary);
        g.setFont(t.monoFont().withHeight(11.4f));
        if (mixA == nullptr || mixB == nullptr)
        {
            g.drawText("Capture Mix A to compare measured spatial field deltas.", footer,
                       juce::Justification::centred);
        }
        else
        {
            g.drawText("Δ Width " + juce::String(mixB->stereo.width - mixA->stereo.width, 2) +
                           " | Δ Corr " +
                           juce::String(mixB->stereo.correlation - mixA->stereo.correlation, 2) +
                           " | Δ Center " +
                           juce::String(mixB->stereo.centerDominance -
                                            mixA->stereo.centerDominance,
                                        2) +
                           " | Δ Motion " +
                           juce::String(mixB->stereo.stereoMotion - mixA->stereo.stereoMotion, 2),
                       footer, juce::Justification::centred);
        }
        return;
    }

    drawSpatialField(g, fieldArea, t, liveMetrics.stereo, t.cyan, 0.88f + 0.10f * pulse, 2.5f);

    if (model->presentationMode == UiPresentationMode::Reference && model->hasReferenceData)
    {
        drawSpatialField(g, fieldArea, t, referenceOverlayMetrics(liveMetrics), t.purple, 0.76f,
                         1.4f);
    }

    if (liveMetrics.stereo.correlation < 0.0f || liveMetrics.stereo.phaseRisk > 0.35f ||
        liveMetrics.stereo.lowBandPhaseRisk > 0.20f)
    {
        g.setColour(t.redWarn.withAlpha(0.26f));
        g.drawRoundedRectangle(fieldArea.expanded(-2.0f, -2.0f), 12.0f, 2.5f);
    }

    const auto warningArea = fieldArea.toNearestInt().removeFromBottom(22);
    if (liveMetrics.stereo.lowBandPhaseRisk > 0.20f)
    {
        g.setColour(t.redWarn.withAlpha(0.18f));
        g.fillRoundedRectangle(warningArea.toFloat().reduced(2.0f, 2.0f), 8.0f);
        g.setColour(t.redWarn);
        g.setFont(t.bodyFont().boldened().withHeight(12.0f));
        g.drawText("Low-end phase risk", warningArea.reduced(8, 0), juce::Justification::centred);
    }
    else if (liveMetrics.stereo.correlation < 0.0f)
    {
        g.setColour(t.redWarn.withAlpha(0.18f));
        g.fillRoundedRectangle(warningArea.toFloat().reduced(2.0f, 2.0f), 8.0f);
        g.setColour(t.redWarn);
        g.setFont(t.bodyFont().boldened().withHeight(12.0f));
        g.drawText("Negative correlation detected", warningArea.reduced(8, 0),
                   juce::Justification::centred);
    }
    else
    {
        const auto stateColour = spatialStateColour(t, liveMetrics.stereo);
        g.setColour(stateColour.withAlpha(0.14f));
        g.fillRoundedRectangle(warningArea.toFloat().reduced(2.0f, 2.0f), 8.0f);
        g.setColour(stateColour);
        g.setFont(t.bodyFont().boldened().withHeight(12.0f));
        g.drawText(spatialStateText(liveMetrics.stereo), warningArea.reduced(8, 0),
                   juce::Justification::centred);
    }

    g.setColour(t.textSecondary);
    g.setFont(t.monoFont().withHeight(11.4f));
    if (model->presentationMode == UiPresentationMode::Reference && model->hasReferenceData)
    {
        g.drawText("Live width " + juce::String(liveMetrics.stereo.width, 2) + " vs ref " +
                       juce::String(liveMetrics.stereo.referenceWidth, 2) + " | Live corr " +
                       juce::String(liveMetrics.stereo.correlation, 2) + " vs ref " +
                       juce::String(liveMetrics.stereo.referenceCorrelation, 2),
                   footer, juce::Justification::centred);
    }
    else
    {
        g.drawText(stereoFooter(liveMetrics), footer, juce::Justification::centred);
    }
}
