#include "StereoArcView.h"

#include "../Plugin/Theme.h"

#include <cmath>

void StereoArcView::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    auto b = getLocalBounds().toFloat().reduced(10);
    const float phase = model ? model->visualPhase : 0.0f;
    const float pulse = model ? model->visualPulse : 0.4f;

    juce::ColourGradient panelGradient(t.panel.brighter(0.04f), b.getX(), b.getY(),
                                       t.panelAlt.darker(0.13f), b.getX(), b.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(b, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(b, t.cornerRadius, 1.0f);

    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(15.5f));
    g.drawText("STEREO IMAGE (WIDTH + CORRELATION)", b.toNearestInt().removeFromTop(30),
               juce::Justification::left);

    auto area = b.reduced(20, 42);
    const auto center = area.getCentre();

    const float mid = model ? model->midEnergy.getCurrentValue() : 0.0f;
    const float side = model ? model->sideEnergy.getCurrentValue() : 0.0f;
    const float corr = model ? model->correlation01.getCurrentValue() : 0.5f;
    const float risk = model ? model->phaseRisk.getCurrentValue() : 0.0f;

    const float baseRadius = juce::jmin(area.getWidth(), area.getHeight()) * 0.35f;
    const float sideRadius = baseRadius * (0.6f + side * 0.8f);
    const float midRadius = baseRadius * (0.35f + mid * 0.55f);

    g.setColour(t.text.withAlpha(0.08f));
    g.drawEllipse(center.x - baseRadius, center.y - baseRadius, baseRadius * 2.0f,
                  baseRadius * 2.0f, 1.0f);
    g.setColour(t.textSecondary.withAlpha(0.18f));
    g.drawEllipse(center.x - sideRadius, center.y - sideRadius * 0.65f, sideRadius * 2.0f,
                  sideRadius * 1.3f, 1.0f);
    g.drawLine(center.x - sideRadius, center.y, center.x + sideRadius, center.y, 1.0f);

    g.setColour(t.teal.withAlpha(0.34f + 0.18f * pulse));
    juce::Path sideArc;
    sideArc.addCentredArc(center.x, center.y, sideRadius, sideRadius * 0.65f, 0.0f,
                          juce::MathConstants<float>::pi * 0.15f,
                          juce::MathConstants<float>::pi * 0.85f, true);
    g.strokePath(sideArc, juce::PathStrokeType(6.0f));

    const float orbitA = phase * 1.2f + juce::MathConstants<float>::pi * 0.2f;
    const juce::Point<float> orbitPoint(center.x + std::cos(orbitA) * sideRadius * 0.85f,
                                        center.y + std::sin(orbitA) * sideRadius * 0.50f);
    const float orbitSize = 2.8f + 1.6f * pulse;
    g.setColour(t.cyan.withAlpha(0.45f + 0.32f * pulse));
    g.fillEllipse(orbitPoint.x - orbitSize, orbitPoint.y - orbitSize, orbitSize * 2.0f,
                  orbitSize * 2.0f);

    g.setColour(t.text.withAlpha(0.4f));
    g.fillEllipse(center.x - midRadius * 0.6f, center.y - midRadius * 0.6f, midRadius * 1.2f,
                  midRadius * 1.2f);

    if (risk > 0.35f || corr < 0.45f)
    {
        g.setColour(t.redWarn.withAlpha(0.35f));
        g.drawEllipse(center.x - sideRadius, center.y - sideRadius * 0.65f, sideRadius * 2.0f,
                      sideRadius * 1.3f, 3.0f);
        g.setColour(t.redWarn.withAlpha(0.8f));
        g.setFont(t.bodyFont().withHeight(13.5f));
        g.drawText("Phase risk: mono compatibility may drop", area.toNearestInt().removeFromBottom(18),
                   juce::Justification::centred);
    }

    auto footer = b.toNearestInt().removeFromBottom(18);
    const auto corrSigned = juce::jmap(corr, 0.0f, 1.0f, -1.0f, 1.0f);
    g.setFont(t.monoFont().withHeight(12.5f));
    g.setColour(t.textSecondary);
    g.drawText("Width " + juce::String(side, 2) + " (0-1)", footer.removeFromLeft(160),
               juce::Justification::left);
    g.drawText("Center focus " + juce::String(mid, 2) + " (mid energy)", footer.removeFromLeft(190),
               juce::Justification::left);
    g.drawText("Correlation " + juce::String(corrSigned, 2) + " (-1..+1)", footer,
               juce::Justification::right);
}
