#include "CandleStoryline.h"

#include "../Plugin/Theme.h"

#include <cmath>

namespace
{
juce::String candleTitle(const UiModel* model)
{
    if (model == nullptr)
    {
        return "SESSION CANDLESTICKS";
    }

    switch (model->presentationMode)
    {
    case UiPresentationMode::Compare:
        return "LIVE CANDLE + LAST 10 DELTAS";
    case UiPresentationMode::Reference:
        return "REFERENCE SESSION CANDLES";
    case UiPresentationMode::Analyze:
    default:
        return "LIVE SESSION CANDLE + LAST 10";
    }
}

juce::Colour bodyColour(const Theme& t, const Candle& candle, bool latest)
{
    const float delta = candle.close - candle.open;
    juce::Colour colour = std::abs(delta) <= 0.015f ? t.gold.withAlpha(0.66f)
                              : delta > 0.0f       ? juce::Colour(0xff3fd08b).withAlpha(0.78f)
                                                   : t.redWarn.withAlpha(0.78f);
    if ((candle.flags & kFlagPhaseRisk) != 0)
    {
        colour = t.redWarn.withAlpha(0.80f);
    }
    if (latest)
    {
        colour = colour.brighter(0.12f);
    }
    return colour;
}

float mapY(const juce::Rectangle<float>& area, float value)
{
    return area.getBottom() - juce::jlimit(0.0f, 1.0f, value) * area.getHeight();
}

void drawSingleCandle(juce::Graphics& g, const juce::Rectangle<float>& area, const Theme& t,
                      const Candle& candle, bool active, bool latest)
{
    const float yHigh = mapY(area, candle.high);
    const float yLow = mapY(area, candle.low);
    const float yOpen = mapY(area, candle.open);
    const float yClose = mapY(area, candle.close);
    const float x = area.getCentreX() - area.getWidth() * 0.14f;
    const float width = area.getWidth() * 0.28f;

    g.setColour(t.text.withAlpha(active ? 0.24f : 0.12f));
    g.drawLine(area.getCentreX(), yHigh, area.getCentreX(), yLow, active ? 2.0f : 1.0f);

    auto body = juce::Rectangle<float>(x, juce::jmin(yOpen, yClose), width,
                                       juce::jmax(4.0f, std::abs(yClose - yOpen)));
    g.setColour(bodyColour(t, candle, latest));
    g.fillRoundedRectangle(body, 4.0f);

    if (active || latest)
    {
        g.setColour(t.cyan.withAlpha(active ? 0.62f : 0.36f));
        g.drawRoundedRectangle(body.expanded(1.0f, 1.0f), 4.0f, 1.0f);
    }
}

} // namespace

void CandleStoryline::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    auto panel = getLocalBounds().toFloat().reduced(10.0f);
    const float phase = model ? model->visualPhase : 0.0f;
    const float pulse = model ? model->visualPulse : 0.0f;

    juce::ColourGradient panelGradient(t.panel.brighter(0.04f), panel.getX(), panel.getY(),
                                       t.panelAlt.darker(0.14f), panel.getX(), panel.getBottom(),
                                       false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panel, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(panel, t.cornerRadius, 1.0f);

    auto bounds = panel.reduced(16.0f, 14.0f);
    auto titleArea = bounds.removeFromTop(20);
    auto helperArea = bounds.removeFromTop(16);
    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened().withHeight(13.5f));
    g.drawText(candleTitle(model), titleArea.toNearestInt(), juce::Justification::left);
    g.setColour(t.textSecondary);
    g.setFont(t.bodyFont().withHeight(11.8f));
    g.drawFittedText(
        "Left plot shows rolling realtime loudness candles. Right plot keeps the last ten finished session candles.",
        helperArea.toNearestInt(), juce::Justification::left, 2);

    bounds.removeFromTop(6.0f);
    auto livePane = bounds.removeFromLeft(juce::jmin(122.0f, bounds.getWidth() * 0.22f));
    bounds.removeFromLeft(8.0f);
    auto historyPane = bounds;

    g.setColour(t.text.withAlpha(0.06f));
    g.drawRoundedRectangle(livePane, 8.0f, 1.0f);
    g.drawRoundedRectangle(historyPane, 8.0f, 1.0f);

    auto liveTitle = livePane.toNearestInt().removeFromTop(18);
    g.setColour(t.textSecondary);
    g.setFont(t.monoFont().withHeight(11.0f));
    g.drawText("REALTIME BUFFER CANDLES", liveTitle, juce::Justification::centred);

    auto livePlot = livePane.reduced(12.0f, 28.0f);
    for (int i = 1; i <= 3; ++i)
    {
        const float y =
            livePlot.getY() + (livePlot.getHeight() * static_cast<float>(i) / 4.0f);
        g.setColour(t.text.withAlpha(0.08f));
        g.drawLine(livePlot.getX(), y, livePlot.getRight(), y, 1.0f);
    }

    bool hasAnyRealtimeCandle = false;
    if (model != nullptr)
    {
        const int count = static_cast<int>(model->realtimeCandleBuf.size());
        const float step = livePlot.getWidth() / static_cast<float>(count);
        for (int i = 0; i < count; ++i)
        {
            const auto& candle = model->realtimeCandleBuf[static_cast<std::size_t>(
                (model->realtimeCandleWrite + i) % count)];
            const bool hasData = candle.t1 > 0.0 || candle.close > 0.0f || candle.high > 0.0f;
            if (!hasData)
            {
                continue;
            }

            hasAnyRealtimeCandle = true;
            auto candleArea = juce::Rectangle<float>(livePlot.getX() + i * step + step * 0.12f,
                                                     livePlot.getY(), step * 0.76f,
                                                     livePlot.getHeight());
            drawSingleCandle(g, candleArea, t, candle, false,
                             !model->liveBufferCandleActive && i == count - 1);
        }

        if (model->liveBufferCandleActive)
        {
            hasAnyRealtimeCandle = true;
            auto candleArea = juce::Rectangle<float>(
                livePlot.getRight() - step + step * 0.12f, livePlot.getY(), step * 0.76f,
                livePlot.getHeight());
            drawSingleCandle(g, candleArea, t, model->liveBufferCandle, true, true);
        }
    }

    if (!hasAnyRealtimeCandle)
    {
        g.setColour(t.textSecondary.withAlpha(0.62f));
        g.setFont(t.bodyFont().withHeight(12.0f));
        g.drawFittedText(model != nullptr && model->analysisStateCode == kAnalysisStateBufferWarming
                             ? "Analysis window is warming before realtime buffer candles open."
                             : "Start playback to open rolling realtime loudness candles.",
                         livePlot.toNearestInt().reduced(6, 6), juce::Justification::centred, 2);
    }

    auto liveFooter = livePane.toNearestInt().removeFromBottom(18);
    g.setColour(t.textSecondary);
    g.setFont(t.monoFont().withHeight(10.8f));
    if (model != nullptr && model->liveBufferCandleActive)
    {
        g.drawText("0.75s windows | TP " + juce::String(model->truePeakDbTP, 2) + " dBTP | Tr " +
                       juce::String(model->transientRateHz, 1) + "/s",
                   liveFooter, juce::Justification::centred);
    }
    else
    {
        g.drawText("Wicks = momentary loudness range | Body = window drift", liveFooter,
                   juce::Justification::centred);
    }

    auto historyTitle = historyPane.toNearestInt().removeFromTop(18);
    g.setColour(t.textSecondary);
    g.drawText("LAST 10 FINISHED SESSIONS", historyTitle, juce::Justification::centredLeft);

    auto plot = historyPane.reduced(10.0f, 26.0f);
    for (int i = 1; i <= 4; ++i)
    {
        const float y = plot.getY() + (plot.getHeight() * static_cast<float>(i) / 5.0f);
        g.setColour(t.text.withAlpha(0.08f));
        g.drawLine(plot.getX(), y, plot.getRight(), y, 1.0f);
    }

    const float scanX = plot.getX() + std::fmod(phase * 95.0f, plot.getWidth());
    juce::ColourGradient scan(t.cyan.withAlpha(0.0f), scanX - 24.0f, plot.getY(),
                              t.cyan.withAlpha(0.18f + 0.12f * pulse), scanX, plot.getCentreY(),
                              false);
    scan.addColour(1.0f, t.cyan.withAlpha(0.0f));
    g.setGradientFill(scan);
    g.fillRect(plot.toNearestInt());

    if (model == nullptr)
    {
        return;
    }

    if (model->firstSessionMixAverage < 0.0f)
    {
        g.setColour(t.textSecondary);
        g.setFont(t.bodyFont().withHeight(12.5f));
        g.drawFittedText("Finished session candles appear here after the first live analysis pass closes.",
                         plot.toNearestInt().reduced(10, 10), juce::Justification::centred, 2);
        return;
    }

    const float baseline = (model->firstSessionMixAverage < 0.0f) ? 0.5f : model->firstSessionMixAverage;
    const float baselineY = plot.getBottom() - baseline * plot.getHeight();
    g.setColour(t.cyan.withAlpha(0.25f));
    g.drawLine(plot.getX(), baselineY, plot.getRight(), baselineY, 1.0f);

    const int count = static_cast<int>(model->candleBuf.size());
    const float step = plot.getWidth() / static_cast<float>(count);
    for (int i = 0; i < count; ++i)
    {
        const auto& candle =
            model->candleBuf[static_cast<std::size_t>((model->candleWrite + i) % count)];
        auto candleArea = juce::Rectangle<float>(plot.getX() + i * step + step * 0.12f, plot.getY(),
                                                 step * 0.76f, plot.getHeight());
        drawSingleCandle(g, candleArea, t, candle, false, i == count - 1);
    }

    const auto latestIndex = (model->candleWrite == 0) ? (model->candleBuf.size() - 1)
                                                        : static_cast<std::size_t>(model->candleWrite - 1);
    const auto& latest = model->candleBuf[latestIndex];
    const float deltaPct = model->lastReferenceDelta * 100.0f;
    juce::String summary = "Plateau detected";
    if ((latest.close - latest.open) > 0.015f)
    {
        summary = "Improving";
    }
    else if ((latest.close - latest.open) < -0.015f)
    {
        summary = "Regression";
    }

    auto footer = historyPane.toNearestInt().removeFromBottom(16);
    g.setColour(t.textSecondary);
    g.setFont(t.monoFont().withHeight(10.8f));
    g.drawText(summary + " | Baseline " + juce::String(juce::roundToInt(baseline * 100.0f)) + "%",
               footer.removeFromLeft(220), juce::Justification::left);
    g.drawText("Session Δ " + juce::String(deltaPct >= 0.0f ? "+" : "") + juce::String(deltaPct, 1) +
                   "% | dLUFS " + juce::String(latest.devLufs, 1),
               footer, juce::Justification::right);
}
