#include "dawai/ui/candlestick_panel.hpp"

#include "dawai/ui/theme/theme.hpp"

#include <cmath>

namespace dawai::ui
{

void CandlestickPanel::setCandles(std::vector<MixCandle> candles, std::size_t writeIndex,
                                  float baseline, float lastDeltaPct)
{
    m_candles = std::move(candles);
    m_writeIndex = writeIndex;
    m_baseline = juce::jlimit(0.0F, 1.0F, baseline);
    m_lastDeltaPct = lastDeltaPct;
    repaint();
}

void CandlestickPanel::setAnimationPhase(float phase, float pulse)
{
    m_animationPhase = phase;
    m_animationPulse = juce::jlimit(0.0F, 1.0F, pulse);
    repaint();
}

void CandlestickPanel::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat();
    const float shimmer = 0.5f + 0.5f * std::sin(m_animationPhase * 1.1f);

    juce::ColourGradient bg(dawai::ui::theme::Palette::panel().brighter(0.04f), panel.getX(),
                            panel.getY(), dawai::ui::theme::Palette::panelAlt().darker(0.15f),
                            panel.getX(), panel.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(panel, 7.0f);
    g.setColour(dawai::ui::theme::Palette::stroke());
    g.drawRoundedRectangle(panel.reduced(1.0f), 7.0f, 1.0f);

    auto bounds = getLocalBounds().reduced(8);

    g.setColour(dawai::ui::theme::Palette::textPrimary());
    g.setFont(dawai::ui::theme::bodyFont().boldened().withHeight(15.5f));
    g.drawText("MIX CANDLESTICK MEMORY", bounds.removeFromTop(20), juce::Justification::left);

    auto plot = bounds.removeFromTop(bounds.getHeight() - 16);
    g.setColour(dawai::ui::theme::Palette::textSecondary().withAlpha(0.10F));
    g.drawRect(plot);

    for (int i = 1; i <= 4; ++i)
    {
        const float y = plot.getY() + (plot.getHeight() * static_cast<float>(i) / 5.0f);
        g.setColour(dawai::ui::theme::Palette::textSecondary().withAlpha(0.08F));
        g.drawLine(plot.getX(), y, plot.getRight(), y, 1.0f);
    }

    const float baselineY = plot.getBottom() - m_baseline * plot.getHeight();
    g.setColour(dawai::ui::theme::Palette::accent().withAlpha(0.24F));
    g.drawLine(plot.getX(), baselineY, plot.getRight(), baselineY, 1.0f);

    const float scanX = plot.getX() + std::fmod(m_animationPhase * 84.0f, juce::jmax(1.0f, (float)plot.getWidth()));
    juce::ColourGradient scan(dawai::ui::theme::Palette::accent().withAlpha(0.0f), scanX - 16.0f,
                              plot.getY(),
                              dawai::ui::theme::Palette::accent().withAlpha(0.16f + 0.10f * shimmer),
                              scanX, plot.getCentreY(), false);
    scan.addColour(1.0f, dawai::ui::theme::Palette::accent().withAlpha(0.0f));
    g.setGradientFill(scan);
    g.fillRect(plot);

    if (!m_candles.empty())
    {
        const int n = static_cast<int>(m_candles.size());
        const float step = plot.getWidth() / static_cast<float>(n);

        auto mapY = [&](float v)
        {
            return plot.getBottom() - juce::jlimit(0.0F, 1.0F, v) * plot.getHeight();
        };

        for (int i = 0; i < n; ++i)
        {
            const auto idx = (m_writeIndex + static_cast<std::size_t>(i)) % m_candles.size();
            const auto& c = m_candles[idx];

            const float x = plot.getX() + i * step + step * 0.2f;
            const float cw = step * 0.6f;

            const float yHigh = mapY(c.high);
            const float yLow = mapY(c.low);
            const float yOpen = mapY(c.open);
            const float yClose = mapY(c.close);

            g.setColour(dawai::ui::theme::Palette::textSecondary().withAlpha(0.14f));
            g.drawLine(x + cw * 0.5f, yHigh, x + cw * 0.5f, yLow, 1.0f);

            const bool up = (c.close >= c.open);
            juce::Colour bodyColour = up ? dawai::ui::theme::Palette::accent().withAlpha(0.72f)
                                         : dawai::ui::theme::Palette::accentWarm().withAlpha(0.72f);
            if ((c.flags & (1u << 4)) != 0)
            {
                bodyColour = dawai::ui::theme::Palette::danger().withAlpha(0.80f);
            }

            const float top = std::min(yOpen, yClose);
            const float bottom = std::max(yOpen, yClose);
            juce::Rectangle<float> body(x, top, cw, juce::jmax(2.0f, bottom - top));
            g.setColour(bodyColour);
            g.fillRoundedRectangle(body, 2.5f);

            if (i == n - 1)
            {
                g.setColour(dawai::ui::theme::Palette::accent().withAlpha(0.45f + 0.25f * m_animationPulse));
                g.drawRoundedRectangle(body.expanded(1.0f), 2.5f, 1.0f);
            }
        }
    }

    g.setColour(dawai::ui::theme::Palette::textSecondary());
    g.setFont(dawai::ui::theme::bodyFont().withHeight(13.0f));
    const auto footer = bounds.removeFromBottom(18);
    const auto deltaText = juce::String(m_lastDeltaPct >= 0.0f ? "+" : "") +
                           juce::String(m_lastDeltaPct, 1) + "%";
    g.drawText("Baseline " + juce::String(m_baseline * 100.0f, 1) + "%", footer,
               juce::Justification::centredLeft);
    g.drawText("Delta " + deltaText, footer, juce::Justification::centredRight);
}

} // namespace dawai::ui
