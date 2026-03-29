#include "dawai/ui/theme/look_and_feel.hpp"

#include "dawai/ui/theme/theme.hpp"

namespace dawai::ui::theme
{

DawLookAndFeel::DawLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, Palette::matteBlack());
    setColour(juce::Label::textColourId, Palette::textPrimary());
    setColour(juce::TextButton::textColourOffId, Palette::textPrimary());
    setColour(juce::TextButton::textColourOnId, Palette::matteBlack());
    setColour(juce::TextButton::buttonColourId, Palette::panelAlt());
    setColour(juce::Slider::thumbColourId, Palette::accent());
    setColour(juce::Slider::trackColourId, Palette::accentMuted());
}

void DawLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour&, bool isMouseOverButton,
                                          bool isButtonDown)
{
    auto c = Palette::panelAlt();
    if (button.getToggleState())
    {
        c = Palette::accent().withAlpha(0.30F);
    }
    if (isMouseOverButton)
    {
        c = c.brighter(0.14F);
    }
    if (isButtonDown)
    {
        c = c.darker(0.15F);
    }

    auto bounds = button.getLocalBounds().toFloat().reduced(1.0F);
    juce::ColourGradient fill(c.brighter(0.08F), bounds.getX(), bounds.getY(), c.darker(0.12F),
                              bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, 6.0F);

    g.setColour(Palette::stroke());
    g.drawRoundedRectangle(bounds, 6.0F, 1.0F);
}

} // namespace dawai::ui::theme
