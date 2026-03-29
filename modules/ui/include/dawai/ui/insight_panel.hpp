#pragma once

#include "dawai/advisory_layer/adviser.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <deque>

namespace dawai::ui
{

class InsightPanel : public juce::Component
{
  public:
    InsightPanel();

    void setOutput(const dawai::advisory_layer::AdvisoryOutput& output);
    void appendStatusMessage(const juce::String& message);
    void setExpanded(bool expanded);
    void setAnimationPhase(float phase, float pulse);

    void paint(juce::Graphics& g) override;
    void resized() override;

  private:
    void appendEntry(const juce::String& summary, const juce::String& fixes, const juce::String& details);
    void rebuildText();

    static constexpr std::size_t kMaxHistoryEntries = 7;
    bool m_expanded = false;
    juce::TextEditor m_summary;
    juce::TextEditor m_fixes;
    juce::TextButton m_toggleDetails{"Show Details"};
    juce::TextEditor m_details;
    std::deque<juce::String> m_summaryHistory;
    std::deque<juce::String> m_fixHistory;
    std::deque<juce::String> m_detailHistory;
    float m_animationPhase = 0.0F;
    float m_animationPulse = 0.0F;
};

} // namespace dawai::ui
