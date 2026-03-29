/*
  ==============================================================================

    ThemeSettingsComponent.cpp
    Created: 29 Mar 2026 12:00:00pm
    Author: Manus

  ==============================================================================
*/

#include "ThemeSettingsComponent.h"

// Plain English Summary: Constructor for the ThemeSettingsComponent.
// Initializes buttons for theme customization.
ThemeSettingsComponent::ThemeSettingsComponent()
{
    // Plain English Summary: Set up the background color button.
    backgroundColorButton.setButtonText ("Background Color");
    addAndMakeVisible (backgroundColorButton);
    backgroundColorButton.onClick = [this]()
    {
        // Plain English Summary: Open a color picker when the button is clicked.
        juce::ColourSelector colourSelector (juce::Colours::black, 0);
        juce::DialogWindow::LaunchOptions options;
        options.content.setNonOwned (&colourSelector);
        options.dialogTitle = "Select Background Color";
        options.dialogBackgroundColour = Theme::instance().bg;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        options.useBottomRightCornerResizer = false;

        options.launchAsync();

        // Plain English Summary: Update the theme's background color when a new color is selected.
        colourSelector.onColourChanged = [this](juce::Colour newColour)
        {
            Theme::instance().setBgColor(newColour);
        };
    };

    // Plain English Summary: Set up the text color button.
    textColorButton.setButtonText ("Text Color");
    addAndMakeVisible (textColorButton);
    textColorButton.onClick = [this]()
    {
        // Plain English Summary: Open a color picker when the button is clicked.
        juce::ColourSelector colourSelector (juce::Colours::white, 0);
        juce::DialogWindow::LaunchOptions options;
        options.content.setNonOwned (&colourSelector);
        options.dialogTitle = "Select Text Color";
        options.dialogBackgroundColour = Theme::instance().bg;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = false;
        options.resizable = false;
        options.useBottomRightCornerResizer = false;

        options.launchAsync();

        // Plain English Summary: Update the theme's text color when a new color is selected.
        colourSelector.onColourChanged = [this](juce::Colour newColour)
        {
            Theme::instance().setTextColor(newColour);
        };
    };

    // Plain English Summary: Set the size of the component.
    setSize (200, 200);
}

// Plain English Summary: Destructor for the ThemeSettingsComponent.
ThemeSettingsComponent::~ThemeSettingsComponent()
{
}

// Plain English Summary: Draws the component's background.
void ThemeSettingsComponent::paint (juce::Graphics& g)
{
    // Plain English Summary: Fill the background with the current theme's background color.
    g.fillAll (Theme::instance().bg.darker(0.2f));
}

// Plain English Summary: Lays out the child components when the component is resized.
void ThemeSettingsComponent::resized()
{
    // Plain English Summary: Position the background color button.
    backgroundColorButton.setBounds (10, 10, getWidth() - 20, 30);
    // Plain English Summary: Position the text color button.
    textColorButton.setBounds (10, 50, getWidth() - 20, 30);
}
