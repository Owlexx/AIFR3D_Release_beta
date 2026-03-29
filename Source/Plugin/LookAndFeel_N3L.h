#pragma once

#include <JuceHeader.h>

#include "Theme.h"

class LookAndFeel_N3L : public juce::LookAndFeel_V4
{
  public:
    LookAndFeel_N3L();
    ~LookAndFeel_N3L() override = default;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool isHovered,
                              bool isDown) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool isHovered, bool isDown) override;

  private:
    Theme& theme = Theme::instance();
};
