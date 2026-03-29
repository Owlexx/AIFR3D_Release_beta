#pragma once

#include <JuceHeader.h>

#include "UiModel.h"

class FixListPanel : public juce::Component
{
  public:
    FixListPanel() = default;

    void setModel(const UiModel& m)
    {
        model = &m;
        repaint();
    }

    void paint(juce::Graphics&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

  private:
    const UiModel* model = nullptr;
    int scrollOffsetPx = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FixListPanel)
};
