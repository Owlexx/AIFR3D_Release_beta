#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dawai::ui::theme
{

class DawLookAndFeel : public juce::LookAndFeel_V4
{
  public:
    DawLookAndFeel();

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                              bool isMouseOverButton, bool isButtonDown) override;
};

} // namespace dawai::ui::theme
