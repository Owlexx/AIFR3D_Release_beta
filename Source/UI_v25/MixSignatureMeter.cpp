#include "MixSignatureMeter.h"

#include "../Plugin/Theme.h"

#include <array>
#include <cmath>

namespace
{
float clampUnit(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

float bandMagnitude(float valueDb)
{
    return juce::jmap(juce::jlimit(-48.0f, 6.0f, valueDb), -48.0f, 6.0f, 0.04f, 1.0f);
}

juce::Colour healthyGreen()
{
    return juce::Colour(0xff34dd7b);
}

struct SignatureAxis
{
    juce::String label;
    float live = 0.0f;
    float reference = 0.0f;
};

std::array<SignatureAxis, 12> buildAxes(const UiModel* model)
{
    if (model == nullptr)
    {
        return {{{"Sub", 0.22f, 0.24f},
                 {"Low-mid", 0.26f, 0.24f},
                 {"Mid", 0.30f, 0.28f},
                 {"Bite", 0.28f, 0.27f},
                 {"Air", 0.20f, 0.18f},
                 {"Dyn", 0.30f, 0.34f},
                 {"Trans", 0.24f, 0.26f},
                 {"Width", 0.22f, 0.24f},
                 {"Center", 0.42f, 0.46f},
                 {"Phase", 0.72f, 0.82f},
                 {"Press", 0.32f, 0.35f},
                 {"Control", 0.52f, 0.58f}}};
    }

    const float refPeakNorm = (model->referenceTruePeakDbTP > -80.0f)
                                  ? clampUnit((model->referenceTruePeakDbTP + 18.0f) / 21.0f)
                                  : 0.62f;
    const float refMomentaryNorm =
        clampUnit((juce::jmax(model->integratedLUFS, model->shortTermLUFS) + 36.0f) / 36.0f);
    const float centerLive = clampUnit(model->midEnergy.getCurrentValue());
    const float centerRef = clampUnit(0.62f + (0.18f * (1.0f - model->stereoWidth)));
    const float phaseLive = clampUnit(0.5f + (0.5f * model->correlation));
    const float phaseRef = clampUnit(0.70f + 0.15f * model->correlation);

    return {{{"Sub", bandMagnitude(model->subBandDb), bandMagnitude(model->referenceSubBandDb)},
             {"Low-mid", bandMagnitude(model->lowMidBandDb), bandMagnitude(model->referenceLowMidBandDb)},
             {"Mid", bandMagnitude(model->midBandDb), bandMagnitude(model->referenceMidBandDb)},
             {"Bite", bandMagnitude(model->highMidBandDb), bandMagnitude(model->referenceHighMidBandDb)},
             {"Air", bandMagnitude(model->airBandDb), bandMagnitude(model->referenceAirBandDb)},
             {"Dyn", clampUnit(model->crestFactorDb / 18.0f), clampUnit(10.0f / 18.0f)},
             {"Trans", clampUnit(model->transientDensity), clampUnit(0.22f + model->transientDensity * 0.35f)},
             {"Width", clampUnit(model->stereoWidth), clampUnit(0.34f)},
             {"Center", centerLive, centerRef},
             {"Phase", phaseLive, phaseRef},
             {"Press", clampUnit((model->momentaryLUFS + 36.0f) / 36.0f), refMomentaryNorm},
             {"Control", clampUnit((model->truePeakDbTP + 18.0f) / 21.0f), refPeakNorm}}};
}

juce::Path buildRadarPath(juce::Rectangle<float> area, const std::array<SignatureAxis, 12>& axes,
                          bool livePath)
{
    juce::Path path;
    const auto center = area.getCentre();
    const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.44f;

    for (std::size_t i = 0; i < axes.size(); ++i)
    {
        const float angle = -juce::MathConstants<float>::halfPi +
                            (juce::MathConstants<float>::twoPi * static_cast<float>(i) /
                             static_cast<float>(axes.size()));
        const float magnitude = livePath ? axes[i].live : axes[i].reference;
        const float r = juce::jmap(magnitude, 0.0f, 1.0f, radius * 0.16f, radius);
        const juce::Point<float> point(center.x + std::cos(angle) * r, center.y + std::sin(angle) * r);
        if (i == 0)
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

void drawMetricCard(juce::Graphics& g, juce::Rectangle<int> area, const Theme& theme,
                    const juce::String& title, const juce::String& subtitle, float meterValue,
                    juce::String readout, juce::Colour accent)
{
    auto card = area.toFloat().reduced(2.0f);
    g.setColour(theme.bg.withAlpha(0.78f));
    g.fillRoundedRectangle(card, 12.0f);
    g.setColour(theme.panelStroke.withAlpha(0.8f));
    g.drawRoundedRectangle(card, 12.0f, 1.0f);

    auto text = area.reduced(10, 8);
    g.setColour(theme.text);
    g.setFont(theme.bodyFont().boldened().withHeight(12.0f));
    g.drawFittedText(title, text.removeFromTop(14), juce::Justification::centredLeft, 1);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(10.8f));
    g.drawFittedText(subtitle, text.removeFromTop(13), juce::Justification::centredLeft, 1);

    auto meter = text.removeFromTop(16).toFloat();
    g.setColour(theme.text.withAlpha(0.08f));
    g.fillRoundedRectangle(meter, 4.0f);
    auto fill = meter.withWidth(meter.getWidth() * clampUnit(meterValue));
    juce::ColourGradient gradient(accent.withAlpha(0.92f), fill.getX(), fill.getCentreY(),
                                  accent.withMultipliedBrightness(1.25f).withAlpha(0.72f),
                                  fill.getRight(), fill.getCentreY(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(fill, 4.0f);

    g.setColour(theme.text);
    g.setFont(theme.titleFont().withHeight(16.0f));
    g.drawFittedText(readout, text, juce::Justification::centredLeft, 1);
}

void drawBandRow(juce::Graphics& g, juce::Rectangle<int> area, const Theme& theme,
                 const juce::String& label, float value, juce::Colour accent, juce::String readout)
{
    auto row = area;
    auto labelArea = row.removeFromLeft(92);
    auto valueArea = row.removeFromRight(52);
    auto barArea = row.toFloat().reduced(4.0f, 3.0f);

    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(11.2f));
    g.drawFittedText(label, labelArea, juce::Justification::centredLeft, 1);

    g.setColour(theme.text.withAlpha(0.07f));
    g.fillRoundedRectangle(barArea, 4.0f);
    auto liveBar = barArea.withWidth(barArea.getWidth() * clampUnit(value));
    g.setColour(accent.withAlpha(0.82f));
    g.fillRoundedRectangle(liveBar, 4.0f);

    g.setColour(theme.textSecondary);
    g.setFont(theme.monoFont().withHeight(10.5f));
    g.drawFittedText(readout, valueArea, juce::Justification::centredRight, 1);
}
} // namespace

void MixSignatureMeter::paint(juce::Graphics& g)
{
    auto& theme = Theme::instance();
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);
    const float pulse = model ? model->visualPulse : 0.35f;
    const float phase = model ? model->visualPhase : 0.0f;

    juce::ColourGradient panelGradient(theme.panel.brighter(0.05f), bounds.getX(), bounds.getY(),
                                       theme.panelAlt.darker(0.12f), bounds.getX(),
                                       bounds.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(bounds, theme.cornerRadius);
    g.setColour(theme.panelStroke);
    g.drawRoundedRectangle(bounds, theme.cornerRadius, 1.0f);

    auto layout = getLocalBounds().reduced(18, 14);
    auto header = layout.removeFromTop(28);
    auto footer = layout.removeFromBottom(18);

    g.setColour(theme.text);
    g.setFont(theme.bodyFont().boldened().withHeight(14.5f));
    g.drawFittedText("MIX SIGNATURE (LIVE SESSION)", header.removeFromLeft(250),
                     juce::Justification::centredLeft, 1);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(12.0f));
    g.drawFittedText("Blue = live mix | Green = reference target",
                     header, juce::Justification::centredRight, 1);

    auto graphPanel = layout.removeFromLeft(static_cast<int>(layout.getWidth() * 0.42f));
    auto infoPanel = layout;

    auto radarArea = graphPanel.toFloat().reduced(12.0f, 6.0f);
    radarArea.removeFromBottom(28.0f);
    radarArea = radarArea.withSizeKeepingCentre(juce::jmin(radarArea.getWidth(), radarArea.getHeight()),
                                                juce::jmin(radarArea.getWidth(), radarArea.getHeight()));

    const auto axes = buildAxes(model);
    const auto livePath = buildRadarPath(radarArea, axes, true);
    const auto referencePath = buildRadarPath(radarArea, axes, false);
    const auto center = radarArea.getCentre();
    const float radius = juce::jmin(radarArea.getWidth(), radarArea.getHeight()) * 0.44f;

    for (int ring = 1; ring <= 4; ++ring)
    {
        const float r = juce::jmap(static_cast<float>(ring), 1.0f, 4.0f, radius * 0.25f, radius);
        g.setColour(theme.text.withAlpha(0.05f));
        g.drawEllipse(center.x - r, center.y - r, r * 2.0f, r * 2.0f, 1.0f);
    }

    for (std::size_t i = 0; i < axes.size(); ++i)
    {
        const float angle = -juce::MathConstants<float>::halfPi +
                            (juce::MathConstants<float>::twoPi * static_cast<float>(i) /
                             static_cast<float>(axes.size()));
        const juce::Point<float> edge(center.x + std::cos(angle) * radius,
                                      center.y + std::sin(angle) * radius);
        g.setColour(theme.text.withAlpha(0.08f));
        g.drawLine(center.x, center.y, edge.x, edge.y, 1.0f);

        const auto labelArea =
            juce::Rectangle<int>(static_cast<int>(edge.x - 30.0f), static_cast<int>(edge.y - 8.0f),
                                 60, 16);
        g.setColour(theme.textSecondary.withAlpha(0.92f));
        g.setFont(theme.monoFont().withHeight(10.5f));
        g.drawFittedText(axes[i].label, labelArea, juce::Justification::centred, 1);
    }

    g.setColour(juce::Colour(0xff38d0ff).withAlpha(0.10f + 0.08f * pulse));
    g.fillPath(livePath);
    g.setColour(healthyGreen().withAlpha(0.08f + 0.04f * (0.5f + 0.5f * std::sin(phase * 0.33f))));
    g.fillPath(referencePath);

    g.setColour(healthyGreen().withAlpha(0.82f));
    g.strokePath(referencePath, juce::PathStrokeType(2.0f));
    g.setColour(juce::Colour(0xff38d0ff).withAlpha(0.18f + 0.10f * pulse));
    g.strokePath(livePath, juce::PathStrokeType(5.0f));
    g.setColour(juce::Colour(0xff38d0ff));
    g.strokePath(livePath, juce::PathStrokeType(2.2f));

    auto legend = graphPanel.removeFromBottom(24);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(11.5f));
    g.drawFittedText("Cyan = Live fingerprint | Green = reference target | LUFS / dBTP / dBFS labels remain visible in the summary",
                     legend, juce::Justification::centredLeft, 1);

    auto summary = infoPanel.removeFromTop(92);
    g.setColour(theme.text);
    g.setFont(theme.bodyFont().withHeight(13.0f));
    g.drawFittedText("Live fingerprint is updating from the current realtime analysis window.",
                     summary.removeFromTop(18), juce::Justification::centredLeft, 1);
    g.setColour(theme.textSecondary);
    g.drawFittedText("Integrated " +
                         (model ? juce::String(model->integratedLUFS, 1) : juce::String("--.-")) +
                         " LUFS | True Peak " +
                         (model ? juce::String(model->truePeakDbTP, 2) : juce::String("--.--")) +
                         " dBTP | Width " +
                         juce::String(model ? model->stereoWidth * 100.0f : 0.0f, 1) + "%",
                     summary.removeFromTop(16), juce::Justification::centredLeft, 1);
    g.drawFittedText("Momentary " +
                         (model ? juce::String(model->momentaryLUFS, 1) : juce::String("--.-")) +
                         " LUFS | Peak " +
                         (model ? juce::String(model->samplePeakDbFS, 2) : juce::String("--.--")) +
                         " dBFS | Volatility " +
                         juce::String(model ? model->visualPulse : 0.0f, 2),
                     summary.removeFromTop(16), juce::Justification::centredLeft, 1);
    g.drawFittedText("Crest " +
                         (model ? juce::String(model->crestFactorDb, 1) : juce::String("--.-")) +
                         " dB | Correlation " +
                         (model ? juce::String(model->correlation, 2) : juce::String("--.--")) +
                         " | Transients " +
                         juce::String(model ? model->transientDensity * 100.0f : 0.0f, 1) + "/s",
                     summary, juce::Justification::centredLeft, 1);

    auto cards = infoPanel.removeFromTop(114);
    const int cardGap = 8;
    const int cardWidth = (cards.getWidth() - cardGap) / 2;
    const int cardHeight = (cards.getHeight() - cardGap) / 2;
    drawMetricCard(g, {cards.getX(), cards.getY(), cardWidth, cardHeight}, theme, "Loudness",
                   "current short-term LUFS",
                   model ? clampUnit((model->shortTermLUFS + 36.0f) / 36.0f) : 0.0f,
                   model ? juce::String(model->shortTermLUFS, 1) + " LUFS" : "--.- LUFS",
                   juce::Colour(0xff38d0ff));
    drawMetricCard(g, {cards.getX() + cardWidth + cardGap, cards.getY(), cardWidth, cardHeight},
                   theme, "True Peak", "live ceiling",
                   model ? clampUnit((model->truePeakDbTP + 18.0f) / 21.0f) : 0.0f,
                   model ? juce::String(model->truePeakDbTP, 2) + " dBTP" : "--.-- dBTP",
                   juce::Colour(0xff38d0ff));
    drawMetricCard(g, {cards.getX(), cards.getY() + cardHeight + cardGap, cardWidth, cardHeight},
                   theme, "Width", "live stereo spread",
                   model ? clampUnit(model->stereoWidth) : 0.0f,
                   juce::String(model ? model->stereoWidth * 100.0f : 0.0f, 1) + "%",
                   juce::Colour(0xff38d0ff));
    drawMetricCard(g, {cards.getX() + cardWidth + cardGap, cards.getY() + cardHeight + cardGap,
                       cardWidth, cardHeight},
                   theme, "Punch", "crest and transient body",
                   model ? clampUnit((model->crestFactorDb * 0.55f + model->transientDensity * 8.0f) / 12.0f)
                         : 0.0f,
                   model ? (juce::String(model->crestFactorDb, 1) + " dB / " +
                            juce::String(model->transientDensity * 100.0f, 0))
                         : "--",
                   juce::Colour(0xff38d0ff));

    infoPanel.removeFromTop(10);
    auto bandsHeader = infoPanel.removeFromTop(30);
    g.setColour(theme.text);
    g.setFont(theme.bodyFont().boldened().withHeight(13.0f));
    g.drawFittedText("Live band energy rows", bandsHeader.removeFromTop(16),
                     juce::Justification::centredLeft, 1);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(11.0f));
    g.drawFittedText("Analyze mode shows live current band energy, not reference-target placeholders.",
                     bandsHeader, juce::Justification::centredLeft, 1);

    const std::array<std::pair<juce::String, float>, 7> bandRows{
        std::make_pair("Sub 20-60", model ? bandMagnitude(model->subBandDb) : 0.0f),
        std::make_pair("Low 60-120", model ? bandMagnitude(model->lowBandDb) : 0.0f),
        std::make_pair("Low-mid 120-400", model ? bandMagnitude(model->lowMidBandDb) : 0.0f),
        std::make_pair("Mid 400-2k", model ? bandMagnitude(model->midBandDb) : 0.0f),
        std::make_pair("High-mid 2k-6k", model ? bandMagnitude(model->highMidBandDb) : 0.0f),
        std::make_pair("High 6k-12k", model ? bandMagnitude(model->highBandDb) : 0.0f),
        std::make_pair("Air 12k-20k", model ? bandMagnitude(model->airBandDb) : 0.0f)};

    const int rowHeight = juce::jmax(16, infoPanel.getHeight() / 8);
    for (const auto& [label, value] : bandRows)
    {
        auto row = infoPanel.removeFromTop(rowHeight);
        drawBandRow(g, row, theme, label, value, juce::Colour(0xff38d0ff),
                    juce::String(juce::jmap(value, 0.0f, 1.0f, -48.0f, 6.0f), 1));
    }

    g.setColour(theme.textSecondary);
    g.setFont(theme.monoFont().withHeight(10.8f));
    g.drawFittedText("Blue/live remains reactive. Green/reference stays steady with slight micro-life.",
                     footer, juce::Justification::centred, 1);
}
