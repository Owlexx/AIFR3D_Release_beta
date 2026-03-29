#pragma once

#include <JuceHeader.h>

#include "UiModel.h"

#include <array>
#include <vector>

class ApprovalHalo : public juce::Component
                   , private juce::Timer
{
  public:
    enum class PulseMode
    {
        Loudness = 1,
        Dynamics,
        Stereo
    };

    enum class DisplayMode
    {
        Live,
        Reference,
        Compare
    };

    enum class BandRegion
    {
        Low,
        Mid,
        High
    };

    struct PulseEvent
    {
        float ageMs = 0.0f;
        float riseMs = 48.0f;
        float holdMs = 96.0f;
        float fadeMs = 260.0f;
        float intensity = 0.0f;
        float radialOffsetPx = 0.0f;
        float thicknessPx = 0.0f;
        float stereoSpread = 0.0f;
        juce::Colour colour;
        BandRegion region = BandRegion::Mid;
    };

    struct HaloVisualState
    {
        juce::Colour toneColour;
        juce::Colour stereoColour;
        juce::Colour loudnessColour;
        juce::Colour dynamicsColour;
        float glowIntensity = 0.18f;
        float pulseRadiusPx = 0.0f;
        float pulseOpacity = 0.0f;
        std::array<float, 7> liveSpectrum{};
        std::array<float, 7> referenceSpectrum{};
        std::vector<PulseEvent> ghostTrails;
    };

    ApprovalHalo();
    ~ApprovalHalo() override = default;

    void setModel(const UiModel& m);
    void setComparisonModel(const UiModel* m);
    void setDisplayMode(DisplayMode mode);

    void setPulseMode(PulseMode nextMode)
    {
        pulseMode = nextMode;
        repaint();
    }

    void paint(juce::Graphics&) override;

  private:
    const UiModel* model = nullptr;
    const UiModel* comparisonModel = nullptr;
    DisplayMode displayMode = DisplayMode::Live;
    PulseMode pulseMode = PulseMode::Loudness;
    HaloVisualState visualState;
    std::vector<PulseEvent> activePulses;
    float lastMomentaryLUFS = -99.0f;
    float lastLowEnergy = 0.0f;
    float lastHighEnergy = 0.0f;
    float lastTransient = 0.0f;
    float lastWidth = 0.0f;
    bool hasSnapshot = false;

    void drawSegment(juce::Graphics& g, juce::Rectangle<float> area, float startAngle,
                     float endAngle, float value, juce::Colour c, float thickness);
    void timerCallback() override;
    void ingestModelSnapshot();
    void updateAnimation(float deltaMs);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApprovalHalo)
};
