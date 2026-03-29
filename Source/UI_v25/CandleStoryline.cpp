#include "CandleStoryline.h"

#include "../Plugin/Theme.h"

#include <algorithm>
#include <cmath>

namespace
{
float clampUnit(float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

template <std::size_t Size>
void drawCandles(juce::Graphics& g, juce::Rectangle<float> area, const Theme& theme,
                 const std::array<Candle, Size>& candles, int writeIndex, juce::String title,
                 juce::String helper, bool emphasiseLatest)
{
    g.setColour(theme.text);
    g.setFont(theme.bodyFont().boldened().withHeight(12.5f));
    g.drawFittedText(title, area.removeFromTop(18).toNearestInt(), juce::Justification::centredLeft, 1);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(10.8f));
    g.drawFittedText(helper, area.removeFromTop(16).toNearestInt(), juce::Justification::centredLeft, 1);

    auto plot = area.reduced(4.0f, 8.0f);
    g.setColour(theme.bg.withAlpha(0.68f));
    g.fillRoundedRectangle(plot, 12.0f);
    g.setColour(theme.panelStroke.withAlpha(0.82f));
    g.drawRoundedRectangle(plot, 12.0f, 1.0f);

    for (int row = 1; row <= 4; ++row)
    {
        const float y = juce::jmap(static_cast<float>(row), 0.0f, 5.0f, plot.getY(), plot.getBottom());
        g.setColour(theme.text.withAlpha(0.05f));
        g.drawLine(plot.getX() + 8.0f, y, plot.getRight() - 8.0f, y, 1.0f);
    }

    const float candleWidth = plot.getWidth() / static_cast<float>(Size);
    for (int index = 0; index < static_cast<int>(Size); ++index)
    {
        const auto& candle = candles[static_cast<std::size_t>((writeIndex + index) % static_cast<int>(Size))];
        const float x = plot.getX() + candleWidth * static_cast<float>(index) + candleWidth * 0.18f;
        const float bodyWidth = candleWidth * 0.62f;
        const auto mapY = [&](float value)
        {
            return juce::jmap(clampUnit(value), 0.0f, 1.0f, plot.getBottom() - 10.0f, plot.getY() + 10.0f);
        };

        const float yHigh = mapY(candle.high);
        const float yLow = mapY(candle.low);
        const float yOpen = mapY(candle.open);
        const float yClose = mapY(candle.close);
        const bool rising = candle.close >= candle.open;
        auto colour = rising ? juce::Colour(0xff38d0ff).withAlpha(0.78f)
                             : theme.redWarn.withAlpha(0.78f);
        if (!rising && candle.close > 0.75f)
        {
            colour = theme.gold.withAlpha(0.78f);
        }

        g.setColour(theme.text.withAlpha(0.16f));
        g.drawLine(x + bodyWidth * 0.5f, yHigh, x + bodyWidth * 0.5f, yLow, 1.2f);

        auto body = juce::Rectangle<float>(x, juce::jmin(yOpen, yClose), bodyWidth,
                                           juce::jmax(3.0f, std::abs(yClose - yOpen)));
        g.setColour(colour);
        g.fillRoundedRectangle(body, 4.0f);

        if (emphasiseLatest && index == static_cast<int>(Size) - 1)
        {
            g.setColour(colour.withAlpha(0.26f));
            g.drawRoundedRectangle(body.expanded(2.0f, 2.0f), 6.0f, 1.2f);
        }
    }
}
} // namespace

void CandleStoryline::paint(juce::Graphics& g)
{
    auto& theme = Theme::instance();
    auto bounds = getLocalBounds().toFloat().reduced(10.0f);

    juce::ColourGradient panelGradient(theme.panel.brighter(0.04f), bounds.getX(), bounds.getY(),
                                       theme.panelAlt.darker(0.14f), bounds.getX(),
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
    g.drawFittedText("LIVE SESSION CANDLE + LAST 10", header.removeFromLeft(240),
                     juce::Justification::centredLeft, 1);
    g.setColour(theme.textSecondary);
    g.setFont(theme.bodyFont().withHeight(11.6f));
    g.drawFittedText("left = rolling realtime deviation | right = finished session history",
                     header, juce::Justification::centredRight, 1);

    auto left = layout.removeFromLeft(static_cast<int>(layout.getWidth() * 0.50f));
    auto right = layout;

    drawCandles(g, left.toFloat(), theme, model ? model->minuteCandleBuf : std::array<Candle, UiModel::kMinuteCandleCount>{},
                model ? model->minuteCandleWrite : 0, "Minute Candles",
                "10 candles across 60 seconds. Each candle = 6 seconds of weighted deviation from the rolling live average.",
                true);

    drawCandles(g, right.toFloat(), theme, model ? model->candleBuf : std::array<Candle, UiModel::kSessionCandleCount>{},
                model ? model->candleWrite : 0, "Last 10 Sessions",
                "Session candles track the most recent finished mixes and retain their measured deviation deltas.",
                true);

    g.setColour(theme.textSecondary);
    g.setFont(theme.monoFont().withHeight(10.8f));
    g.drawFittedText("Minute chart is driven by momentary LUFS, true peak, width, spectral tilt, and transient energy. Session chart persists the last ten completed sessions.",
                     footer, juce::Justification::centred, 1);
}
