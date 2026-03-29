#pragma once

#include "dawai/ui/insight_panel.hpp"
#include "dawai/ui/meter_panel.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace dawai::ui
{

class Aifr3dPanel : public juce::Component
{
  public:
    enum class Mode
    {
        True,
        Compare,
        Reference
    };

    Aifr3dPanel();

    void setMode(Mode mode);
    void setMeterSnapshot(const dawai::metering::MeterSnapshot& snapshot);
    void setReferenceTruePeakDbtp(double value);
    void setInsight(const dawai::advisory_layer::AdvisoryOutput& output);
    void appendInsightStatus(const juce::String& message);
    void setAnimationPhase(float phase, float pulse);

    void resized() override;
    void paint(juce::Graphics& g) override;

  private:
    Mode m_mode = Mode::True;

    juce::TextButton m_trueButton{"TRUE"};
    juce::TextButton m_compareButton{"COMPARE"};
    juce::TextButton m_referenceButton{"REFERENCE"};
    juce::TextButton m_pickPremixButton{"Premix File"};
    juce::TextButton m_pickMixedButton{"Mixed File"};
    juce::TextButton m_analyzeRendersButton{"Analyze"};
    juce::Label m_compareStatus;

    MeterPanel m_meterPanel;
    InsightPanel m_insightPanel;
    float m_animationPhase = 0.0F;
    float m_animationPulse = 0.0F;
};

} // namespace dawai::ui
