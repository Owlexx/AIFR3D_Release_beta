#include "LookAndFeel_N3L.h"

LookAndFeel_N3L::LookAndFeel_N3L()
{
    setColour(juce::Label::textColourId, theme.text);
    setColour(juce::TextButton::buttonColourId, theme.panelAlt);
    setColour(juce::TextButton::textColourOffId, theme.text);
    setColour(juce::TextButton::textColourOnId, theme.bg);
}

void LookAndFeel_N3L::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                           const juce::Colour& base, bool hovered, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    auto c = base.isTransparent() ? theme.panelAlt : base;
    if (b.getToggleState())
        c = theme.teal.withAlpha(0.30f);

    if (down)
        c = c.darker(0.20f);
    if (hovered)
        c = c.brighter(0.08f);

    juce::ColourGradient fill(c.brighter(0.08f), r.getX(), r.getY(), c.darker(0.12f), r.getX(),
                              r.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(r, theme.cornerRadius * 0.52f);

    g.setColour(theme.panelStroke);
    g.drawRoundedRectangle(r, theme.cornerRadius * 0.52f, 1.0f);
}

void LookAndFeel_N3L::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool hovered,
                                     bool down)
{
    juce::ignoreUnused(hovered, down);
    g.setColour(button.getToggleState() ? theme.bg : theme.text);
    g.setFont(theme.bodyFont().boldened().withHeight(14.5f));
    g.drawText(button.getButtonText(), button.getLocalBounds().reduced(6, 1),
               juce::Justification::centred, false);
}
