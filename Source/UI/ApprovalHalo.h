#pragma once

#include <JuceHeader.h>

#include "UiModel.h"

class ApprovalHalo : public juce::Component
{
  public:
    enum class PulseMode
    {
        Loudness = 1,
        Dynamics,
        Stereo
    };

    ApprovalHalo() = default;

    void setModel(const UiModel& m)
    {
        model = &m;
    }

    void setPulseMode(PulseMode nextMode)
    {
        pulseMode = nextMode;
        repaint();
    }

    void paint(juce::Graphics&) override;

  private:
    const UiModel* model = nullptr;
    PulseMode pulseMode = PulseMode::Loudness;

    void drawSegment(juce::Graphics& g, juce::Rectangle<float> area, float startAngle,
                     float endAngle, float value, juce::Colour c, float thickness);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApprovalHalo)
};
