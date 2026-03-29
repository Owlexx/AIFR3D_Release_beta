#include "ApprovalHalo.h"

#include "../Plugin/Theme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>

namespace
{
float clampUnit(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float easeOutCubic(float x)
{
    const float t = 1.0f - clampUnit(x);
    return 1.0f - (t * t * t);
}

float easeInOutSine(float x)
{
    const float t = clampUnit(x);
    return 0.5f - (0.5f * std::cos(t * juce::MathConstants<float>::pi));
}

float bandMagnitude(float valueDb)
{
    return juce::jmap(juce::jlimit(-48.0f, 6.0f, valueDb), -48.0f, 6.0f, 0.04f, 1.0f);
}

float averageRange(std::initializer_list<float> values)
{
    if (values.size() == 0)
    {
        return 0.0f;
    }

    float sum = 0.0f;
    for (const auto value : values)
    {
        sum += value;
    }
    return sum / static_cast<float>(values.size());
}

juce::Colour deepBlue()
{
    return juce::Colour(0xff0b4d9b);
}

juce::Colour healthyGreen()
{
    return juce::Colour(0xff34dd7b);
}

juce::Colour hotCyan()
{
    return juce::Colour(0xff3cd3ff);
}

juce::Colour warmLowAlert()
{
    return juce::Colour(0xffe66f4a);
}

juce::Colour toneTargetColour(const UiModel* model)
{
    if (model == nullptr)
    {
        return healthyGreen();
    }

    const float lowMidBody = model->lowMidBandDb - model->midBandDb;
    const float upperPush = juce::jmax(model->highMidBandDb - model->midBandDb,
                                       model->highBandDb - model->highMidBandDb);
    if (lowMidBody > 2.0f || model->spectralBalance < -1.8f)
    {
        return deepBlue();
    }
    if (upperPush > 1.5f || model->spectralBalance > 1.4f)
    {
        return hotCyan();
    }
    return healthyGreen();
}

juce::Colour stereoTargetColour(const UiModel* model)
{
    if (model == nullptr)
    {
        return healthyGreen();
    }

    if (model->stereoWidth < 0.16f || model->correlation > 0.92f)
    {
        return deepBlue();
    }
    if (model->stereoWidth > 0.58f || model->correlation < 0.0f)
    {
        return hotCyan();
    }
    return healthyGreen();
}

juce::Colour loudnessTargetColour(const UiModel* model)
{
    if (model == nullptr)
    {
        return healthyGreen();
    }

    if (model->momentaryLUFS < -20.0f || model->integratedLUFS < -18.0f)
    {
        return deepBlue();
    }
    if (model->momentaryLUFS > -9.5f || model->truePeakDbTP > -1.0f)
    {
        return hotCyan();
    }
    return healthyGreen();
}

juce::Colour dynamicsTargetColour(const UiModel* model)
{
    if (model == nullptr)
    {
        return healthyGreen();
    }

    if (model->crestFactorDb > 14.5f)
    {
        return deepBlue();
    }
    if (model->crestFactorDb < 7.0f || model->transientDensity > 0.42f)
    {
        return hotCyan();
    }
    return healthyGreen();
}

float pulseEnvelope(const ApprovalHalo::PulseEvent& event)
{
    if (event.ageMs < event.riseMs)
    {
        return easeOutCubic(event.ageMs / juce::jmax(1.0f, event.riseMs));
    }
    if (event.ageMs < (event.riseMs + event.holdMs))
    {
        return 1.0f;
    }

    const float fadeAge = event.ageMs - event.riseMs - event.holdMs;
    if (fadeAge >= event.fadeMs)
    {
        return 0.0f;
    }
    return 1.0f - easeInOutSine(fadeAge / juce::jmax(1.0f, event.fadeMs));
}

std::array<float, 7> liveSpectrumTargets(const UiModel* model)
{
    if (model == nullptr)
    {
        return {0.20f, 0.24f, 0.28f, 0.26f, 0.22f, 0.18f, 0.14f};
    }

    return {bandMagnitude(model->subBandDb),     bandMagnitude(model->lowBandDb),
            bandMagnitude(model->lowMidBandDb),  bandMagnitude(model->midBandDb),
            bandMagnitude(model->highMidBandDb), bandMagnitude(model->highBandDb),
            bandMagnitude(model->airBandDb)};
}

std::array<float, 7> referenceSpectrumTargets(const UiModel* model)
{
    if (model == nullptr)
    {
        return {0.18f, 0.22f, 0.26f, 0.26f, 0.22f, 0.18f, 0.14f};
    }

    return {bandMagnitude(model->referenceSubBandDb),     bandMagnitude(model->referenceLowBandDb),
            bandMagnitude(model->referenceLowMidBandDb),  bandMagnitude(model->referenceMidBandDb),
            bandMagnitude(model->referenceHighMidBandDb), bandMagnitude(model->referenceHighBandDb),
            bandMagnitude(model->referenceAirBandDb)};
}

juce::String formatSigned(float value, int decimals = 1)
{
    return juce::String(value >= 0.0f ? "+" : "") + juce::String(value, decimals);
}

void drawMetricCell(juce::void drawMetricCell(juce::Graphics& g, juce::Rectangle<int> area, const Theme& theme,
                    const juce::String& title, const juce::String& value,
                    const juce::String& helper, juce::Colour accent, const juce::String& scale = {})
{
    auto card = area.toFloat().reduced(2.0f, 2.0f);
    g.setColour(theme.bg.withAlpha(0.78f));
    g.fillRoundedRectangle(card, 12.0f);
    g.setColour(theme.panelStroke.withAlpha(0.8f));
    g.drawRoundedRectangle(card, 12.0f, 1.0f);

    auto text = area.reduced(10, 8);
    auto header = text.removeFromTop(16);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().boldened().withHeight(12.0f));
    g.drawFittedText(title, header.removeFromLeft(header.getWidth() / 2), juce::Justification::centredLeft, 1);
    
    if (scale.isNotEmpty())
    {
        g.setColour(theme.textSecondary.withAlpha(0.5f));
        g.setFont(theme.monoFont().withHeight(10.0f));
        g.drawFittedText(scale, header, juce::Justification::centredRight, 1);
    }

    g.setColour(accent);
    g.setFont(theme.titleFont().withHeight(24.0f).boldened());
    g.drawFittedText(value, text.removeFromTop(28), juce::Justification::centredLeft, 1);

    g.setColour(theme.textSecondary.withAlpha(0.85f));
    g.setFont(theme.bodyFont().withHeight(11.5f));
    g.drawFittedText(helper, text, juce::Justification::centredLeft, 2);
}nt().withHeight(10.8f));
    g.drawFittedText(helper, text, juce::Justification::centredLeft, 2);
}

void drawSpectrumTrace(juce::Graphics& g, juce::Rectangle<float> area, const Theme& theme,
                       const std::array<float, 7>& liveValues,
                       const std::array<float, 7>& referenceValues, float glow)
{
    const auto makePoints = [&](const std::array<float, 7>& values)
    {
        std::array<juce::Point<float>, 7> points{};
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            const float x = juce::jmap(static_cast<float>(i), 0.0f, 6.0f, area.getX(), area.getRight());
            const float y = juce::jmap(values[i], 0.0f, 1.0f, area.getBottom(), area.getY());
            points[i] = {x, y};
        }
        return points;
    };

    const auto makePath = [](const std::array<juce::Point<float>, 7>& points)
    {
        juce::Path path;
        path.startNewSubPath(points.front());
        for (std::size_t i = 1; i < points.size(); ++i)
        {
            const auto previous = points[i - 1];
            const auto current = points[i];
            const float midX = (previous.x + current.x) * 0.5f;
            path.cubicTo({midX, previous.y}, {midX, current.y}, current);
        }
        return path;
    };

    for (int row = 1; row <= 3; ++row)
    {
        const float y =
            juce::jmap(static_cast<float>(row), 0.0f, 4.0f, area.getY(), area.getBottom());
        g.setColour(theme.text.withAlpha(0.05f));
        g.drawLine(area.getX(), y, area.getRight(), y, 1.0f);
    }

    const auto referencePath = makePath(makePoints(referenceValues));
    const auto livePath = makePath(makePoints(liveValues));

    g.setColour(healthyGreen().withAlpha(0.22f));
    g.strokePath(referencePath, juce::PathStrokeType(1.8f));

    g.setColour(theme.cyan.withAlpha(0.08f + 0.06f * glow));
    g.strokePath(livePath, juce::PathStrokeType(5.0f));
    g.setColour(theme.cyan.withAlpha(0.82f));
    g.strokePath(livePath, juce::PathStrokeType(2.1f));
}

void drawPulseArc(juce::Graphics& g, juce::Rectangle<float> area, float startAngle, float endAngle,
                  float radiusOffset, float thickness, juce::Colour colour, float envelope)
{
    juce::Path arc;
    auto ring = area.expanded(radiusOffset);
    ring = ring.reduced(thickness * 0.5f);
    arc.addCentredArc(ring.getCentreX(), ring.getCentreY(), ring.getWidth() * 0.5f,
                      ring.getHeight() * 0.5f, 0.0f, startAngle, endAngle, true);
    g.setColour(colour.withAlpha(envelope));
    g.strokePath(arc, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
}
} // namespace

ApprovalHalo::ApprovalHalo()
{
    activePulses.reserve(12);
    visualState.ghostTrails.reserve(12);
    visualState.toneColour = healthyGreen();
    visualState.stereoColour = healthyGreen();
    visualState.loudnessColour = healthyGreen();
    visualState.dynamicsColour = healthyGreen();
    visualState.liveSpectrum = liveSpectrumTargets(nullptr);
    visualState.referenceSpectrum = referenceSpectrumTargets(nullptr);
    startTimerHz(60);
}

void ApprovalHalo::setModel(const UiModel& m)
{
    model = &m;
    ingestModelSnapshot();
    repaint();
}

void ApprovalHalo::setComparisonModel(const UiModel* m)
{
    comparisonModel = m;
    repaint();
}

void ApprovalHalo::setDisplayMode(DisplayMode mode)
{
    displayMode = mode;
    repaint();
}

void ApprovalHalo::timerCallback()
{
    constexpr float kFrameMs = 1000.0f / 60.0f;
    ingestModelSnapshot();
    updateAnimation(kFrameMs);

    if (isShowing())
    {
        repaint();
    }
}

void ApprovalHalo::ingestModelSnapshot()
{
    const auto toneTarget = toneTargetColour(model);
    const auto stereoTarget = stereoTargetColour(model);
    const auto loudnessTarget = loudnessTargetColour(model);
    const auto dynamicsTarget = dynamicsTargetColour(model);
    visualState.toneColour = visualState.toneColour.interpolatedWith(toneTarget, 0.12f);
    visualState.stereoColour = visualState.stereoColour.interpolatedWith(stereoTarget, 0.12f);
    visualState.loudnessColour = visualState.loudnessColour.interpolatedWith(loudnessTarget, 0.12f);
    visualState.dynamicsColour = visualState.dynamicsColour.interpolatedWith(dynamicsTarget, 0.12f);

    const auto liveTarget = liveSpectrumTargets(model);
    const auto referenceTarget = referenceSpectrumTargets(model);
    for (std::size_t i = 0; i < liveTarget.size(); ++i)
    {
        visualState.liveSpectrum[i] += (liveTarget[i] - visualState.liveSpectrum[i]) * 0.18f;
        visualState.referenceSpectrum[i] +=
            (referenceTarget[i] - visualState.referenceSpectrum[i]) * 0.07f;
    }

    if (model == nullptr)
    {
        return;
    }

    const float lowEnergy =
        averageRange({liveTarget[0], liveTarget[1], liveTarget[2]});
    const float midEnergy =
        averageRange({liveTarget[2], liveTarget[3], liveTarget[4]});
    const float highEnergy =
        averageRange({liveTarget[4], liveTarget[5], liveTarget[6]});
    const float transient = clampUnit(model->transientDensity);
    const float rmsNorm = clampUnit((model->rmsDbFS + 36.0f) / 36.0f);
    const float momentaryDelta = hasSnapshot
                                     ? clampUnit(std::abs(model->momentaryLUFS - lastMomentaryLUFS) / 4.5f)
                                     : 0.0f;
    const float lowSpike = hasSnapshot ? clampUnit((lowEnergy - lastLowEnergy) * 7.0f) : 0.0f;
    const float highSpike = hasSnapshot ? clampUnit((highEnergy - lastHighEnergy) * 9.0f) : 0.0f;
    const float widthDelta = hasSnapshot ? std::abs(model->stereoWidth - lastWidth) : 0.0f;

    if (!hasSnapshot)
    {
        lastMomentaryLUFS = model->momentaryLUFS;
        lastLowEnergy = lowEnergy;
        lastHighEnergy = highEnergy;
        lastTransient = transient;
        lastWidth = model->stereoWidth;
        hasSnapshot = true;
        return;
    }

    auto addPulse = [this](float intensity, BandRegion region, juce::Colour colour, float riseMs,
                           float holdMs, float fadeMs, float radialOffset, float thickness,
                           float spread)
    {
        if (intensity < 0.24f)
        {
            return;
        }

        PulseEvent event;
        event.intensity = clampUnit(intensity);
        event.region = region;
        event.colour = colour;
        event.riseMs = riseMs;
        event.holdMs = holdMs;
        event.fadeMs = fadeMs;
        event.radialOffsetPx = radialOffset;
        event.thicknessPx = thickness;
        event.stereoSpread = spread;

        if (activePulses.size() >= 12)
        {
            activePulses.erase(activePulses.begin());
        }
        activePulses.push_back(event);

        auto ghost = event;
        ghost.intensity *= 0.42f;
        ghost.radialOffsetPx += 7.0f;
        ghost.thicknessPx *= 0.82f;
        ghost.fadeMs += 80.0f;
        ghost.colour = ghost.colour.withMultipliedBrightness(0.92f);
        if (visualState.ghostTrails.size() >= 12)
        {
            visualState.ghostTrails.erase(visualState.ghostTrails.begin());
        }
        visualState.ghostTrails.push_back(ghost);
    };

    float lowPulse = clampUnit(0.38f * lowEnergy + 0.24f * transient + 0.22f * rmsNorm +
                               0.16f * juce::jmax(lowSpike, momentaryDelta));
    float midPulse = clampUnit(0.36f * midEnergy + 0.22f * transient + 0.20f * rmsNorm +
                               0.22f * clampUnit(widthDelta * 9.0f + momentaryDelta));
    float highPulse = clampUnit(0.34f * highEnergy + 0.26f * transient + 0.20f * rmsNorm +
                                0.20f * juce::jmax(highSpike, momentaryDelta));

    if (pulseMode == PulseMode::Loudness)
    {
        lowPulse *= 1.14f;
    }
    else if (pulseMode == PulseMode::Dynamics)
    {
        midPulse *= 1.10f;
        highPulse *= 1.08f;
    }
    else
    {
        midPulse *= 1.12f;
    }

    if (lowSpike > 0.04f || lowPulse > 0.52f || (transient - lastTransient) > 0.08f)
    {
        juce::Colour lowColour = visualState.loudnessColour;
        if (lowEnergy > (midEnergy + 0.08f))
        {
            lowColour = lowColour.interpolatedWith(warmLowAlert(), 0.38f);
        }
        addPulse(lowPulse, BandRegion::Low, lowColour, 56.0f, 110.0f, 320.0f,
                 5.0f + (12.0f * lowEnergy), 8.0f + (8.0f * lowEnergy), clampUnit(model->stereoWidth));
    }

    if (momentaryDelta > 0.10f || widthDelta > 0.03f || midPulse > 0.50f)
    {
        addPulse(midPulse, BandRegion::Mid, visualState.stereoColour, 48.0f, 92.0f, 250.0f,
                 3.0f + (8.0f * midEnergy), 6.0f + (6.0f * midEnergy), clampUnit(model->stereoWidth));
    }

    if (highSpike > 0.025f || highPulse > 0.46f)
    {
        addPulse(highPulse, BandRegion::High, visualState.toneColour.interpolatedWith(hotCyan(), 0.25f),
                 42.0f, 84.0f, 210.0f, 4.0f + (8.0f * highEnergy), 5.0f + (5.0f * highEnergy),
                 clampUnit(model->stereoWidth));
    }

    lastMomentaryLUFS = model->momentaryLUFS;
    lastLowEnergy = lowEnergy;
    lastHighEnergy = highEnergy;
    lastTransient = transient;
    lastWidth = model->stereoWidth;
}

void ApprovalHalo::updateAnimation(float deltaMs)
{
    const auto updateList = [deltaMs](std::vector<PulseEvent>& events)
    {
        for (auto& event : events)
        {
            event.ageMs += deltaMs;
        }
        events.erase(std::remove_if(events.begin(), events.end(),
                                    [](const PulseEvent& event)
                                    {
                                        return event.ageMs >
                                               (event.riseMs + event.holdMs + event.fadeMs);
                                    }),
                     events.end());
    };

    updateList(activePulses);
    updateList(visualState.ghostTrails);

    float strongest = 0.0f;
    float widest = 0.0f;
    for (const auto& event : activePulses)
    {
        const float envelope = pulseEnvelope(event);
        strongest = juce::jmax(strongest, envelope * event.intensity);
        widest = juce::jmax(widest, envelope * (event.radialOffsetPx + event.thicknessPx));
    }

    visualState.glowIntensity +=
        ((0.16f + strongest * 0.78f) - visualState.glowIntensity) * 0.16f;
    visualState.pulseOpacity += ((0.08f + strongest * 0.70f) - visualState.pulseOpacity) * 0.18f;
    visualState.pulseRadiusPx += ((widest * 0.65f) - visualState.pulseRadiusPx) * 0.20f;
}

void ApprovalHalo::drawSegment(juce::Graphics& g, juce::Rectangle<float> area, float startA,
                               float endA, float value, juce::Colour c, float thickness)
{
    juce::Path p;
    auto r = area.reduced(thickness * 0.5f);
    p.addCentredArc(r.getCentreX(), r.getCentreY(), r.getWidth() * 0.5f, r.getHeight() * 0.5f, 0.0f,
                    startA, juce::jmap(value, 0.0f, 1.0f, startA, endA), true);
    g.setColour(c);
    g.strokePath(p, juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
}

void ApprovalHalo::paint(juce::Graphics& g)
{
    auto& theme = Theme::instance();
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);

    juce::ColourGradient panelGradient(theme.panel.brighter(0.05f), bounds.getX(), bounds.getY(),
                                       theme.panelAlt.darker(0.16f), bounds.getX(),
                                       bounds.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(bounds, theme.cornerRadius);
    g.setColour(theme.panelStroke);
    g.drawRoundedRectangle(bounds, theme.cornerRadius, 1.0f);

    auto layout = getLocalBounds().reduced(18, 14);
    auto header = layout.removeFromTop(28);
    auto footer = layout.removeFromBottom(18);

    g.setColour(theme.text);
    g.setFont(theme.bodyFont().boldened().withHeight(14.0f));
    g.drawFittedText("HALO METER", header.removeFromLeft(140), juce::Justification::centredLeft, 1);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(12.0f));
    g.drawFittedText(model ? model->haloDetail : juce::String("Waiting for live buffer"),
                     header, juce::Justification::centredRight, 1);

    juce::Rectangle<int> ringPanel;
    juce::Rectangle<int> metricPanel;
    if (layout.getWidth() > layout.getHeight() * 1.08f)
    {
        ringPanel = layout.removeFromLeft(static_cast<int>(layout.getWidth() * 0.58f));
        metricPanel = layout;
    }
    else
    {
        ringPanel = layout.removeFromTop(static_cast<int>(layout.getHeight() * 0.58f));
        metricPanel = layout;
    }

    auto ringArea = ringPanel.toFloat().reduced(8.0f);
    ringArea = ringArea.withSizeKeepingCentre(juce::jmin(ringArea.getWidth(), ringArea.getHeight()),
                                              juce::jmin(ringArea.getWidth(), ringArea.getHeight()));
    const auto analyzerArea =
        ringArea.reduced(ringArea.getWidth() * 0.26f, ringArea.getHeight() * 0.31f);

    g.setColour(theme.text.withAlpha(0.05f));
    for (int index = 1; index <= 3; ++index)
    {
        const float inset = static_cast<float>(index) * 10.0f;
        g.drawEllipse(ringArea.reduced(inset), 1.0f);
    }

    drawSpectrumTrace(g, analyzerArea, theme, visualState.liveSpectrum, visualState.referenceSpectrum,
                      visualState.glowIntensity);

    const float overall = model ? model->mixAlignment.getCurrentValue() : 0.0f;
    const float toneValue = model ? model->tone.getCurrentValue() : 0.0f;
    const float stereoValue = model ? model->space.getCurrentValue() : 0.0f;
    const float loudnessValue =
        model ? clampUnit(0.55f * clampUnit((model->momentaryLUFS + 24.0f) / 24.0f) +
                          0.45f * clampUnit(1.0f - juce::jmax(0.0f, model->truePeakDbTP + 1.0f) / 3.0f))
              : 0.0f;
    const float dynamicsValue = model ? model->dynamics.getCurrentValue() : 0.0f;

    const float start = -juce::MathConstants<float>::pi * 0.75f;
    const float seg = juce::MathConstants<float>::halfPi;
    const float gap = seg * 0.12f;
    const float thickness = 12.0f;

    auto pulseArea = ringArea.expanded(visualState.pulseRadiusPx + 10.0f);
    for (const auto& trail : visualState.ghostTrails)
    {
        const float envelope = pulseEnvelope(trail) * trail.intensity * 0.36f;
        if (envelope <= 0.0f)
        {
            continue;
        }

        if (trail.region == BandRegion::Low)
        {
            drawPulseArc(g, pulseArea, start + 2.0f * seg + gap, start + 3.0f * seg - gap,
                         trail.radialOffsetPx + 8.0f, trail.thicknessPx + 3.0f, trail.colour,
                         envelope * 0.50f);
        }
        else if (trail.region == BandRegion::High)
        {
            drawPulseArc(g, pulseArea, start + gap, start + seg - gap, trail.radialOffsetPx + 8.0f,
                         trail.thicknessPx + 2.5f, trail.colour, envelope * 0.48f);
        }
        else
        {
            drawPulseArc(g, pulseArea, start + seg + gap, start + 2.0f * seg - gap,
                         trail.radialOffsetPx + 8.0f, trail.thicknessPx + 2.0f, trail.colour,
                         envelope * 0.40f);
            drawPulseArc(g, pulseArea, start + 3.0f * seg + gap, start + 4.0f * seg - gap,
                         trail.radialOffsetPx + 8.0f, trail.thicknessPx + 2.0f, trail.colour,
                         envelope * 0.40f);
        }
    }

    for (const auto& pulse : activePulses)
    {
        const float envelope = pulseEnvelope(pulse) * pulse.intensity;
        if (envelope <= 0.0f)
        {
            continue;
        }

        if (pulse.region == BandRegion::Low)
        {
            drawPulseArc(g, pulseArea, start + 2.0f * seg + gap, start + 3.0f * seg - gap,
                         pulse.radialOffsetPx, pulse.thicknessPx, pulse.colour, envelope * 0.72f);
        }
        else if (pulse.region == BandRegion::High)
        {
            drawPulseArc(g, pulseArea, start + gap, start + seg - gap, pulse.radialOffsetPx,
                         pulse.thicknessPx, pulse.colour, envelope * 0.78f);
        }
        else
        {
            const float spread = juce::jmap(pulse.stereoSpread, 0.0f, 1.0f, 0.10f, 0.22f);
            drawPulseArc(g, pulseArea, start + seg + gap, start + 2.0f * seg - gap - spread,
                         pulse.radialOffsetPx, pulse.thicknessPx, pulse.colour, envelope * 0.60f);
            drawPulseArc(g, pulseArea, start + 3.0f * seg + gap + spread,
                         start + 4.0f * seg - gap, pulse.radialOffsetPx, pulse.thicknessPx,
                         pulse.colour, envelope * 0.60f);
        }
    }

    drawSegment(g, ringArea, start + 0.0f * seg + gap, start + 1.0f * seg - gap, toneValue,
                visualState.toneColour, thickness);
    drawSegment(g, ringArea, start + 1.0f * seg + gap, start + 2.0f * seg - gap, stereoValue,
                visualState.stereoColour, thickness);
    drawSegment(g, ringArea, start + 2.0f * seg + gap, start + 3.0f * seg - gap, loudnessValue,
                visualState.loudnessColour, thickness);
    drawSegment(g, ringArea, start + 3.0f * seg + gap, start + 4.0f * seg - gap, dynamicsValue,
                visualState.dynamicsColour, thickness);

    const float coreRadius = ringArea.getWidth() * 0.25f;
    juce::Colour coreColour = visualState.toneColour.interpolatedWith(
        visualState.loudnessColour, 0.5f);
    juce::ColourGradient glow(coreColour.withAlpha(0.18f + (0.26f * visualState.glowIntensity)),
                              ringArea.getCentreX(), ringArea.getCentreY(),
                              theme.bg.withAlpha(0.04f), ringArea.getX(), ringArea.getBottom(),
                              true);
    g.setGradientFill(glow);
    g.fillEllipse(ringArea.getCentreX() - coreRadius, ringArea.getCentreY() - coreRadius,
                  coreRadius * 2.0f, coreRadius * 2.0f);

    g.setColour(theme.text.withAlpha(0.86f));
    g.setFont(theme.bodyFont().boldened().withHeight(12.0f));
    g.drawFittedText("TONE", juce::Rectangle<int>(ringArea.getCentreX() - 42, ringArea.getY() - 18, 84, 14),
                     juce::Justification::centred, 1);
    g.drawFittedText("STEREO",
                     juce::Rectangle<int>(ringArea.getRight() + 2, ringArea.getCentreY() - 8, 72, 14),
                     juce::Justification::centredLeft, 1);
    g.drawFittedText("LOW / LOUDNESS",
                     juce::Rectangle<int>(ringArea.getCentreX() - 70, ringArea.getBottom() + 4, 140, 14),
                     juce::Justification::centred, 1);
    g.drawFittedText("DYNAMICS",
                     juce::Rectangle<int>(ringArea.getX() - 82, ringArea.getCentreY() - 8, 80, 14),
                     juce::Justification::centredRight, 1);

    g.setColour(theme.text);
    g.setFont(theme.bodyFont().boldened().withHeight(13.0f));
    g.drawFittedText(model ? model->haloLabel : juce::String("LIVE"), ringArea.toNearestInt().translated(0, -46),
                     juce::Justification::centred, 1);
    g.setFont(theme.titleFont().withHeight(26.0f));
    g.drawFittedText(juce::String(juce::roundToInt(overall * 100.0f)) + "%",
                     ringArea.toNearestInt().translated(0, -4), juce::Justification::centred, 1);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(12.0f));
    g.drawFittedText("Pulse fade tracks transient, RMS, LUFS, spectrum, and width",
                     ringArea.toNearestInt().translated(0, 32), juce::Justification::centred, 2);

    auto metricArea = metricPanel.reduced(4);
    const int columns = 2;
    const int rows = 4;
    const int gapPx = 6;
    const int cellWidth = (metricArea.getWidth() - gapPx) / columns;
    const int cellHeight = (metricArea.getHeight() - (gapPx * (rows - 1))) / rows;

    const float widthPercent = model ? (model->stereoWidth * 100.0f) : 0.0f;
    const auto toneAccent = visualState.toneColour;
    const auto stereoAccent = visualState.stereoColour;
    const auto loudnessAccent = visualState.loudnessColour;
    const auto dynamicsAccent = visualState.dynamicsColour;

    const std::array<std::tuple<juce::String, juce::String, juce::String, juce::Colour>, 8> cells{
        std::make_tuple("M LUFS",
                        model ? juce::String(model->momentaryLUFS, 1) + " LUFS" : "--.-- LUFS",
                        "400 ms momentary loudness", loudnessAccent),
        std::make_tuple("S LUFS",
                        model ? juce::String(model->shortTermLUFS, 1) + " LUFS" : "--.-- LUFS",
                        "3 second loudness window", loudnessAccent),
        std::make_tuple("I LUFS",
                        model ? juce::String(model->integratedLUFS, 1) + " LUFS" : "--.-- LUFS",
                        "session integrated loudness", loudnessAccent),
        std::make_tuple("True Peak",
                        model ? juce::String(model->truePeakDbTP, 2) + " dBTP" : "--.-- dBTP",
                        "oversampled ceiling", loudnessAccent),
        std::make_tuple("RMS",
                        model ? juce::String(model->rmsDbFS, 1) + " dBFS" : "--.-- dBFS",
                        "display-smoothed program level", dynamicsAccent),
        std::make_tuple("Width",
                        juce::String(widthPercent, 1) + " %",
                        "mid-side stereo spread", stereoAccent),
        std::make_tuple("Correlation",
                        model ? juce::String(model->correlation, 2) : "--.--",
                        "mono compatibility", stereoAccent),
        std::make_tuple("Crest Factor",
                        model ? juce::String(model->crestFactorDb, 1) + " dB" : "--.-- dB",
                        "peak versus RMS headroom", dynamicsAccent)};

    for (int index = 0; index < static_cast<int>(cells.size()); ++index)
    {
        const int row = index / columns;
        const int column = index % columns;
        const int x = metricArea.getX() + column * (cellWidth + gapPx);
        const int y = metricArea.getY() + row * (cellHeight + gapPx);
        const auto& cell = cells[static_cast<std::size_t>(index)];
        drawMetricCell(g, {x, y, cellWidth, cellHeight}, theme, std::get<0>(cell), std::get<1>(cell),
                       std::get<2>(cell), std::get<3>(cell));
    }

    g.setColour(theme.textSecondary);
    g.setFont(theme.monoFont().withHeight(10.8f));
    g.drawFittedText("Deep blue = quiet or muddy | Green = balanced | Cyan = hot or bright",
                     footer, juce::Justification::centred, 1);
}
