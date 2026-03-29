#pragma once

#include <JuceHeader.h>

#include "UiModel.h"

#include <functional>

class TopBar : public juce::Component
{
  public:
    enum class Mode
    {
        Analyze,
        Compare,
        Reference
    };

    TopBar();

    void setModel(const UiModel& m)
    {
        model = &m;
    }

    void paint(juce::Graphics&) override;
    void resized() override;
    void setLayoutVariant(int variant);
    [[nodiscard]] int layoutVariant() const noexcept
    {
        return selectedLayoutVariant;
    }
    [[nodiscard]] Mode mode() const noexcept
    {
        return activeMode;
    }

    std::function<void(Mode)> onModeChanged;
    std::function<void(int)> onLayoutVariantChanged;
    std::function<void(bool)> onVersionToggleChanged;

  private:
    juce::Image tryLoadLogoFile(const juce::File& file) const;
    juce::Image tryLoadLogoFromDirectory(const juce::File& dir) const;
    juce::Image loadBrandLogo() const;
    static juce::Image downscaleLogo(const juce::Image& source);
    void updateModeSelection(juce::Button* source);

    const UiModel* model = nullptr;
    juce::Image brandLogo;
    juce::String activeModeLabel{"ANALYZE"};
    Mode activeMode = Mode::Analyze;
    int selectedLayoutVariant = 1;

    juce::TextButton compareBtn{"Compare"};
    juce::TextButton refBtn{"Reference"};
    juce::TextButton approvalBtn{"Analyze"};
    juce::ToggleButton demoToggle{"Demo"};
    juce::Label layoutLabel;
    juce::ComboBox layoutBox;
    juce::ToggleButton versionToggle{"V2.2.5 Mode"};
    juce::ToggleButton compareModeToggle{"Compare Mode"};
    juce::TextButton themeSettingsButton{"Theme Settings"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBar)
};
