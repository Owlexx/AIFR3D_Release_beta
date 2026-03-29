/*
  ==============================================================================

    StandaloneVocalRecorder.h
    Created: 29 Mar 2026 10:00:00pm
    Author: Manus

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Source/Plugin/Theme.h"
#include "../../Source/UI_v25/ApprovalHalo.h"
#include "../../Source/UI_v25/MixSignatureMeter.h"
#include "AIVocalProcessor.h"

//==============================================================================
/*
    This class implements the main application window for the Standalone Vocal Recorder.
    It manages the GUI, audio processing, and AI-driven vocal effects.
*/
class StandaloneVocalRecorderApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    StandaloneVocalRecorderApplication();

    const juce::String getApplicationName() override       { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override;
    void shutdown() override;

    void systemRequestedQuit() override;

    //==============================================================================
    /*
        This window serves as the main container for the standalone application's GUI.
        It hosts the vocal recording interface, meters, and AI feedback.
    */
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name);

        void closeButtonPressed() override;

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    Theme laf;
};

//==============================================================================
/*
    This class represents the main content component for the Standalone Vocal Recorder.
    It will contain the recording controls, AI feedback, and visual meters.
*/
class StandaloneVocalRecorderComponent  : public juce::AudioAppComponent
{
public:
    StandaloneVocalRecorderComponent();
    ~StandaloneVocalRecorderComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Plain English Summary: This Halo meter visualizes the overall tonal balance, stereo width, loudness, and dynamics of the vocal input.
    ApprovalHalo vocalHalo;
    // Plain English Summary: This meter displays the historical trend of loudness, peak, width, and dynamic range for the vocal performance.
    MixSignatureMeter vocalCandlestick;

    // Plain English Summary: The audio processor responsible for AI-driven vocal effects and analysis.
    AIVocalProcessor audioProcessor;

    // Plain English Summary: Button to start or stop audio input/output.
    juce::TextButton startAndStopButton;
    // Plain English Summary: Component to configure audio input and output devices.
    juce::AudioDeviceSelectorComponent audioSetupComp { getAudioDeviceManager().getDeviceSetup(),
                                                        getAudioDeviceManager().getWantsMidiInput(),
                                                        getAudioDeviceManager().getWantsMidiOutput(),
                                                        false, false, true, false };

    void startOrStopAudio();

    // Plain English Summary: Manages the WebSocket connection to the backend for real-time vocal DSP data.
    std::unique_ptr<juce::WebSockets::WebSocket> webSocket;
    void connectWebSocket();
    void disconnectWebSocket();
    void sendVocalDspMetrics(const audiosynth::dsp::AnalysisFrame& frame);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneVocalRecorderComponent)
};
