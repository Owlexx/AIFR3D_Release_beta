#include "FixListPanel.h"

#include "../Plugin/Theme.h"

#include <cmath>

namespace
{
constexpr auto kMissingKeyFeedback = "OpenAI API key missing — AIFR3D chat unavailable";
constexpr auto kOfflineFeedback = "OpenAI request failed — AIFR3D chat unavailable";
}

void FixListPanel::paint(juce::Graphics& g)
{
    auto& t = Theme::instance();
    auto b = getLocalBounds().toFloat().reduced(10);
    const float phase = model ? model->visualPhase : 0.0f;
    const float pulse = model ? model->visualPulse : 0.4f;
    const float shimmer = 0.5f + 0.5f * std::sin(phase * 0.9f);

    juce::ColourGradient panelGradient(t.panel.brighter(0.04f), b.getX(), b.getY(),
                                       t.panelAlt.darker(0.14f), b.getX(), b.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(b, t.cornerRadius);
    g.setColour(t.panelStroke);
    g.drawRoundedRectangle(b, t.cornerRadius, 1.0f);

    g.setColour(t.text);
    g.setFont(t.bodyFont().boldened());
    g.drawText("FIX LIST", b.toNearestInt().removeFromTop(28), juce::Justification::left);

    auto area = b.reduced(16, 36);

    const auto* cards = model != nullptr && !model->fixList.empty()
                            ? &model->fixList
                            : (model != nullptr ? &model->diagnosticFixList : nullptr);

    if (!model || cards == nullptr || cards->empty())
    {
        scrollOffsetPx = 0;
        juce::String message;
        if (model != nullptr)
        {
            const auto status = model->statusText.trim();
            if (status == kMissingKeyFeedback || status == kOfflineFeedback)
            {
                message = status;
            }
        }
        else
        {
            message = kMissingKeyFeedback;
        }

        if (message.isNotEmpty())
        {
            g.setColour(t.text.withAlpha(0.45f));
            g.setFont(t.bodyFont());
            g.drawText(message, area.toNearestInt(), juce::Justification::centred);
        }
        return;
    }

    constexpr int cardH = 128;
    constexpr int cardSpacing = 6;
    const int contentHeight = static_cast<int>(cards->size()) * (cardH + cardSpacing);
    const int viewportHeight = static_cast<int>(area.getHeight());
    const int maxScroll = juce::jmax(0, contentHeight - viewportHeight);
    scrollOffsetPx = juce::jlimit(0, maxScroll, scrollOffsetPx);

    int y = static_cast<int>(area.getY()) - scrollOffsetPx;

    g.saveState();
    g.reduceClipRegion(area.toNearestInt());

    for (size_t i = 0; i < cards->size(); ++i)
    {
        auto r = juce::Rectangle<int>(static_cast<int>(area.getX()), y,
                                      static_cast<int>(area.getWidth()), cardH)
                     .reduced(0, 6);
        y += cardH + cardSpacing;

        if (r.getBottom() < static_cast<int>(area.getY()) ||
            r.getY() > static_cast<int>(area.getBottom()))
        {
            continue;
        }

        g.setColour(t.bg.withAlpha(0.84f + 0.06f * shimmer));
        g.fillRoundedRectangle(r.toFloat(), t.cornerRadius * 0.6f);
        g.setColour(t.panelStroke);
        g.drawRoundedRectangle(r.toFloat(), t.cornerRadius * 0.6f, 1.0f);

        g.setColour(t.text);
        g.setFont(t.bodyFont().boldened().withHeight(15.0f));
        g.drawText((*cards)[i].title, r.removeFromTop(22), juce::Justification::left);

        g.setColour(t.text.withAlpha(0.75f));
        g.setFont(t.bodyFont().withHeight(13.5f));
        g.drawText("Why: " + (*cards)[i].why, r.removeFromTop(24), juce::Justification::left);
        g.drawText("Next: " + (*cards)[i].next, r.removeFromTop(24), juce::Justification::left);

        auto meta = r;
        auto badge = meta.removeFromLeft(70).reduced(0, 2);
        const auto impact = (*cards)[i].impact.toLowerCase();
        const bool highImpact = impact.contains("high");
        g.setColour((highImpact ? t.gold : t.cyan).withAlpha(0.20f + 0.10f * pulse));
        g.fillRoundedRectangle(badge.toFloat(), 8.0f);
        g.setColour((highImpact ? t.gold : t.cyan).withAlpha(0.80f + 0.20f * shimmer));
        g.setFont(t.bodyFont().boldened().withHeight(12.5f));
        g.drawText(highImpact ? "HIGH" : "MED", badge, juce::Justification::centred);

        meta.removeFromLeft(8);
        g.setColour(t.textSecondary);
        g.setFont(t.monoFont().withHeight(12.0f));
        g.drawText("Signal clarity " + juce::String((*cards)[i].certainty, 2) +
                       " | Impact " + (*cards)[i].impact,
                   meta, juce::Justification::left);
    }
    g.restoreState();

    if (maxScroll > 0)
    {
        const float ratio = static_cast<float>(viewportHeight) / static_cast<float>(contentHeight);
        const float thumbH = juce::jmax(22.0f, area.getHeight() * ratio);
        const float trackH = area.getHeight();
        const float scrollT = static_cast<float>(scrollOffsetPx) / static_cast<float>(maxScroll);
        const float thumbY = area.getY() + scrollT * (trackH - thumbH);
        const auto track =
            juce::Rectangle<float>(area.getRight() - 4.0f, area.getY(), 3.0f, area.getHeight());
        const auto thumb = juce::Rectangle<float>(track.getX(), thumbY, track.getWidth(), thumbH);
        g.setColour(t.panelStroke.withAlpha(0.55f));
        g.fillRoundedRectangle(track, 2.0f);
        g.setColour(t.cyan.withAlpha(0.72f));
        g.fillRoundedRectangle(thumb, 2.0f);
    }
}

void FixListPanel::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const int delta = static_cast<int>(-wheel.deltaY * 64.0f);
    scrollOffsetPx = juce::jmax(0, scrollOffsetPx + delta);
    repaint();
}
