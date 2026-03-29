#pragma once

#include <JuceHeader.h>

#include "UiModel.h"

class MixSignatureMeter : public juce::Component
{
  public:
    MixSignatureMeter() = default;

    void setModel(const UiModel& m)
    {
        model = &m;
        repaint();
    }

    void paint(juce::Graphics&) override;

  private:
    const UiModel* model = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixSignatureMeter)
};
