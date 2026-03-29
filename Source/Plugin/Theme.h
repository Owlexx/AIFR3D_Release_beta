/*
  ==============================================================================

    Theme.h
    Created: 29 Mar 2026 12:00:00pm
    Author: Manus

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#if !defined(AUDIOSYNTH_UI_VARIANT)
#define AUDIOSYNTH_UI_VARIANT 1
#endif

// Plain English Summary: This class manages the visual theme of the plugin, including colors and fonts.
// It is a singleton, meaning only one instance exists, and it can notify other components when the theme changes.
class Theme : public juce::ChangeBroadcaster
{
public:
    // Plain English Summary: Returns the single instance of the Theme class.
    static Theme& instance();

    // Plain English Summary: Default constructor, initializes theme colors.
    Theme();

    // Plain English Summary: Sets the background color of the theme and notifies listeners.
    void setBgColor(juce::Colour newColor);
    // Plain English Summary: Sets the text color of the theme and notifies listeners.
    void setTextColor(juce::Colour newColor);
    // Add setters for other colors as needed

    // Plain English Summary: Retrieves the main background color.
    juce::Colour getBgColor() const { return bg; }
    // Plain English Summary: Retrieves the main text color.
    juce::Colour getTextColor() const { return text; }
    // Add getters for other colors as needed

    juce::Colour bg;
    juce::Colour text;
    juce::Colour textSecondary;
    juce::Colour panel;
    juce::Colour panelAlt;
    juce::Colour panelStroke;
    juce::Colour teal;
    juce::Colour cyan;
    juce::Colour gold;
    juce::Colour purple;

    float cornerRadius = 6.0f;

    juce::Font titleFont() const;
    juce::Font bodyFont() const;
    juce::Font monoFont() const;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Theme)
};
