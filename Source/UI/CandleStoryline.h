#pragma once

#include <JuceHeader.h>

#include "UiModel.h"

class CandleStoryline : public juce::Component
{
  public:
    CandleStoryline() = default;

    void setModel(const UiModel& m)
    {
        model = &m;
    }

    void paint(juce::Graphics&) override;

  private:
    const UiModel* model = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CandleStoryline)
};
