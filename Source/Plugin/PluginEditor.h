#pragma once

#include <JuceHeader.h>

#include "../UI/MainView.h"
#include "../UI_v25/MainView.h"
#include "LookAndFeel_N3L.h"
#include "PluginProcessor.h"

#include <filesystem>
#include <memory>

class CoreSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::ChangeListener
{
  public:
    explicit CoreSynthAudioProcessorEditor(CoreSynthAudioProcessor&);
    ~CoreSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void updateHaloLayout();


  private:
    void showStartupSplash();
    void maybeShowFirstRunTutorial();
    static std::filesystem::path tutorialFlagPath();

    CoreSynthAudioProcessor& processor;
    LookAndFeel_N3L laf;
    MainView mainView;
    std::unique_ptr<MainView_v25> mainViewV25;
    std::unique_ptr<ApprovalHalo> haloA;
    std::unique_ptr<ApprovalHalo> haloB;
    bool isV25Mode = false;
    bool isCompareMode = false;

    void setCompareMode(bool compareMode);

    std::unique_ptr<juce::Component> splashOverlay;
    std::unique_ptr<juce::Component> tutorialOverlay;

    // Plain English Summary: Applies the current theme colors to all UI components.
    void applyTheme();

    // Plain English Summary: Manages the WebSocket connection to the backend for real-time DSP data.
    std::unique_ptr<juce::WebSockets::WebSocket> webSocket;
    void connectWebSocket();
    void disconnectWebSocket();
    void sendDspMetrics(const audiosynth::dsp::AnalysisFrame& frame);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoreSynthAudioProcessorEditor)
};
