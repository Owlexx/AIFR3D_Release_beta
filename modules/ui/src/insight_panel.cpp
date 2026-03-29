#include "dawai/ui/insight_panel.hpp"

#include "dawai/ui/theme/theme.hpp"

#include <cmath>

namespace dawai::ui
{

InsightPanel::InsightPanel()
{
    addAndMakeVisible(m_summary);
    addAndMakeVisible(m_fixes);
    addAndMakeVisible(m_toggleDetails);
    addAndMakeVisible(m_details);

    m_summary.setMultiLine(true);
    m_summary.setReadOnly(true);
    m_summary.setScrollbarsShown(true);
    m_summary.setCaretVisible(false);
    m_summary.setPopupMenuEnabled(false);
    m_summary.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    m_summary.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    m_summary.setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    m_summary.setColour(juce::TextEditor::textColourId, dawai::ui::theme::Palette::textPrimary());
    m_summary.applyFontToAllText(dawai::ui::theme::bodyFont().boldened().withHeight(15.0f));

    m_fixes.setMultiLine(true);
    m_fixes.setReadOnly(true);
    m_fixes.setScrollbarsShown(true);
    m_fixes.setCaretVisible(false);
    m_fixes.setPopupMenuEnabled(false);
    m_fixes.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    m_fixes.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    m_fixes.setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    m_fixes.setColour(juce::TextEditor::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_fixes.applyFontToAllText(dawai::ui::theme::bodyFont().withHeight(14.0f));

    m_details.setMultiLine(true);
    m_details.setReadOnly(true);
    m_details.setScrollbarsShown(true);
    m_details.setCaretVisible(false);
    m_details.setPopupMenuEnabled(false);
    m_details.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    m_details.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    m_details.setColour(juce::TextEditor::shadowColourId, juce::Colours::transparentBlack);
    m_details.setColour(juce::TextEditor::textColourId, dawai::ui::theme::Palette::textSecondary());
    m_details.applyFontToAllText(dawai::ui::theme::bodyFont().withHeight(13.5f));
    m_details.setVisible(false);

    m_toggleDetails.onClick = [this] { setExpanded(!m_expanded); };
}

void InsightPanel::paint(juce::Graphics& g)
{
    auto panel = getLocalBounds().toFloat();
    const float shimmer = 0.5f + 0.5f * std::sin(m_animationPhase * 1.1f);
    juce::ColourGradient bg(dawai::ui::theme::Palette::panelAlt().brighter(0.03f), panel.getX(),
                            panel.getY(), dawai::ui::theme::Palette::panelAlt().darker(0.18f),
                            panel.getX(), panel.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(panel, 8.0f);
    g.setColour(dawai::ui::theme::Palette::stroke());
    g.drawRoundedRectangle(panel.reduced(1.0f), 8.0f, 1.0f);
    g.setColour(dawai::ui::theme::Palette::accent().withAlpha(0.04f + 0.10f * m_animationPulse));
    g.drawRoundedRectangle(panel.reduced(2.0f), 8.0f, 1.0f);

    g.setColour(dawai::ui::theme::Palette::textSecondary());
    g.setFont(dawai::ui::theme::bodyFont().boldened().withHeight(12.5f));
    g.drawText("INSIGHT", getLocalBounds().removeFromTop(20).reduced(8, 0),
               juce::Justification::left);
    const float scanX =
        panel.getX() + std::fmod(m_animationPhase * 58.0f, juce::jmax(24.0f, panel.getWidth()));
    juce::ColourGradient line(
        dawai::ui::theme::Palette::accent().withAlpha(0.0f), scanX - 16.0f, panel.getY() + 18.0f,
        dawai::ui::theme::Palette::accent().withAlpha(0.16f + 0.10f * shimmer), scanX,
        panel.getY() + 18.0f, false);
    line.addColour(1.0, dawai::ui::theme::Palette::accent().withAlpha(0.0f));
    g.setGradientFill(line);
    g.fillRect((int)panel.getX() + 8, (int)panel.getY() + 18, (int)panel.getWidth() - 16, 1);
}

void InsightPanel::setOutput(const dawai::advisory_layer::AdvisoryOutput& output)
{
    juce::String fixes;
    if (!output.issues.empty())
    {
        for (const auto& issue : output.issues)
        {
            const auto summary = juce::String(issue.summary).trim();
            if (summary.isEmpty())
            {
                continue;
            }

            fixes << "- " << summary << "\n";
            if (!issue.tryFirst.empty())
            {
                const auto firstMove = juce::String(issue.tryFirst.front()).trim();
                if (firstMove.isNotEmpty())
                {
                    fixes << "  " << firstMove << "\n";
                }
            }
        }
    }
    else
    {
        for (const auto& fix : output.top3Fixes)
        {
            fixes << "- " << fix << "\n";
        }
    }

    appendEntry(juce::String(output.summary30s), fixes, juce::String(output.details));
}

void InsightPanel::appendStatusMessage(const juce::String& message)
{
    appendEntry(message, {}, message);
}

void InsightPanel::setExpanded(bool expanded)
{
    m_expanded = expanded;
    m_toggleDetails.setButtonText(expanded ? "Hide Details" : "Show Details");
    m_details.setVisible(expanded);
    resized();
}

void InsightPanel::setAnimationPhase(float phase, float pulse)
{
    m_animationPhase = phase;
    m_animationPulse = juce::jlimit(0.0F, 1.0F, pulse);
    repaint();
}

void InsightPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(12);
    m_summary.setBounds(area.removeFromTop(88));
    m_fixes.setBounds(area.removeFromTop(116));
    m_toggleDetails.setBounds(area.removeFromTop(30));
    if (m_expanded)
    {
        m_details.setBounds(area);
    }
}

void InsightPanel::appendEntry(const juce::String& summary, const juce::String& fixes,
                               const juce::String& details)
{
    const auto pushBounded = [](std::deque<juce::String>& queue, const juce::String& value)
    {
        if (value.trim().isEmpty())
        {
            return;
        }
        queue.push_back(value.trim());
        while (queue.size() > kMaxHistoryEntries)
        {
            queue.pop_front();
        }
    };

    pushBounded(m_summaryHistory, summary);
    pushBounded(m_fixHistory, fixes);
    pushBounded(m_detailHistory, details);
    rebuildText();
}

void InsightPanel::rebuildText()
{
    auto joinHistory = [](const std::deque<juce::String>& queue)
    {
        juce::String out;
        int entryIndex = 1;
        for (const auto& entry : queue)
        {
            if (out.isNotEmpty())
            {
                out << "\n\n";
            }
            out << "#" << juce::String(entryIndex++) << "  " << entry;
        }
        return out;
    };

    m_summary.setText(joinHistory(m_summaryHistory), juce::dontSendNotification);
    m_fixes.setText(joinHistory(m_fixHistory), juce::dontSendNotification);
    m_details.setText(joinHistory(m_detailHistory), juce::dontSendNotification);
    m_summary.moveCaretToEnd();
    m_fixes.moveCaretToEnd();
    m_details.moveCaretToEnd();
}

} // namespace dawai::ui
