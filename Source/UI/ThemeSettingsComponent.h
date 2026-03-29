/*
  ==============================================================================

    ThemeSettingsComponent.h
    Created: 29 Mar 2026 12:00:00pm
    Author: Manus

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../Plugin/Theme.h"

// Plain English Summary: This component provides a user interface for customizing the plugin's theme.
// It allows users to change colors of various UI elements and potentially rearrange windows.
class ThemeSettingsComponent  : public juce::Component
{
public:
    ThemeSettingsComponent();
    ~ThemeSettingsComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Plain English Summary: Button to open a color picker for background color.
    juce::TextButton backgroundColorButton;
    // Plain English Summary: Button to open a color picker for text color.
    juce::TextButton textColorButton;
    // Add more buttons/sliders for other theme properties as needed

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemeSettingsComponent)
};
