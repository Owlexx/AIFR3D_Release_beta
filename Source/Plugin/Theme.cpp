/*
  ==============================================================================

    Theme.cpp
    Created: 29 Mar 2026 12:00:00pm
    Author: Manus

  ==============================================================================
*/

#include "Theme.h"

// Plain English Summary: Returns the single instance of the Theme class.
Theme& Theme::instance()
{
    static Theme t;
    return t;
}

// Plain English Summary: Constructor for the Theme class. Initializes default colors and fonts.
Theme::Theme()
    : bg (juce::Colour::fromRGB(8, 10, 14)),
      text (juce::Colour::fromRGB(238, 242, 250)),
      textSecondary (juce::Colour::fromRGB(140, 140, 140)),
      panel (juce::Colour::fromRGB(16, 20, 28)),
      panelAlt (juce::Colour::fromRGB(12, 16, 23)),
      panelStroke (juce::Colour::fromRGB(40, 40, 40)),
      teal (juce::Colour::fromRGB(0, 200, 200)),
      cyan (juce::Colour::fromRGB(0, 255, 255)),
      gold (juce::Colour::fromRGB(255, 200, 0)),
      purple (juce::Colour::fromRGB(150, 0, 200))
{
}

// Plain English Summary: Sets the background color of the theme and notifies all registered listeners.
void Theme::setBgColor(juce::Colour newColor)
{
    if (bg != newColor)
    {
        bg = newColor;
        sendChangeMessage(); // Notify listeners that the theme has changed
    }
}

// Plain English Summary: Sets the text color of the theme and notifies all registered listeners.
void Theme::setTextColor(juce::Colour newColor)
{
    if (text != newColor)
    {
        text = newColor;
        sendChangeMessage(); // Notify listeners that the theme has changed
    }
}

// Plain English Summary: Returns the font for titles, bolded and with a specific height.
juce::Font Theme::titleFont() const
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), 32.0f, juce::Font::bold);
}

// Plain English Summary: Returns the font for body text.
juce::Font Theme::bodyFont() const
{
    return juce::Font (juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::plain);
}

// Plain English Summary: Returns the monospaced font for code or technical text.
juce::Font Theme::monoFont() const
{
    return juce::Font (
        juce::Font::findBestMatchingSansSerifTypefaceFor(
            juce::StringArray({"JetBrains Mono", "IBM Plex Mono", "Source Code Pro", "Menlo"}),
            juce::Font::getDefaultMonospacedFontName()),
        13.0f, juce::Font::plain);
}
