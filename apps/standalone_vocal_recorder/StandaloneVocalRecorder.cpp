/*
  ==============================================================================

    StandaloneVocalRecorder.cpp
    Created: 29 Mar 2026 10:00:00pm
    Author: Manus

  ==============================================================================
*/

#include "StandaloneVocalRecorder.h"

//==============================================================================
StandaloneVocalRecorderComponent::StandaloneVocalRecorderComponent()
    : juce::AudioAppComponent(), audioProcessor()
{
    // Plain English Summary: Add the vocal Halo meter to the component and make it visible.
    addAndMakeVisible (vocalHalo);
    // Plain English Summary: Add the vocal Candlestick meter to the component and make it visible.
    addAndMakeVisible (vocalCandlestick);

    // Plain English Summary: Add and make visible the audio setup component.
    addAndMakeVisible(audioSetupComp);
    // Plain English Summary: Add and make visible the start/stop button for audio.
    addAndMakeVisible(startAndStopButton);

    // Plain English Summary: Set up the start/stop button's text and action.
    startAndStopButton.setButtonText("Start/Stop Audio");
    startAndStopButton.onClick = [this] { startOrStopAudio(); };

    // Plain English Summary: Set the format manager to use default audio devices.
    setAudioChannels(2, 2);
    connectWebSocket(); // Plain English Summary: Connects to the backend WebSocket for real-time vocal DSP data streaming.
}

StandaloneVocalRecorderComponent::~StandaloneVocalRecorderComponent()
{
    shutdownAudio();
    disconnectWebSocket(); // Plain English Summary: Disconnects the WebSocket when the component is destroyed.
}

void StandaloneVocalRecorderComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // Plain English Summary: Prepare the audio processor for playback.
    audioProcessor.prepareToPlay(sampleRate, samplesPerBlockExpected);
}

void StandaloneVocalRecorderComponent::releaseResources()
{
    // Plain English Summary: Release resources from the audio processor.
    audioProcessor.releaseResources();
}

void StandaloneVocalRecorderComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Plain English Summary: Pass the audio block to the AI vocal processor for processing.
    juce::MidiBuffer midiMessages;
    audioProcessor.processBlock(*bufferToFill.buffer, midiMessages);

    // Plain English Summary: Update the vocal Halo and Candlestick meters with the latest DSP analysis from the AI Vocal Processor.
    aifred::dsp::AnalysisFrame currentFrame = audioProcessor.getAnalysisFrame();
    vocalHalo.update (currentFrame);
    vocalCandlestick.update (currentFrame);

    // Send vocal DSP metrics over WebSocket to the backend for AI processing
    sendVocalDspMetrics(currentFrame);
}

void StandaloneVocalRecorderComponent::paint (juce::Graphics& g)
{
    // Plain English Summary: Fill the background of the component with the theme's background color.
    g.fillAll (Theme::instance().bg);
}

void StandaloneVocalRecorderComponent::resized()
{
    // Plain English Summary: Position the vocal Halo meter in the top-left quadrant of the component.
    vocalHalo.setBounds (getLocalBounds().removeFromLeft(getWidth() / 2).removeFromTop(getHeight() / 2));
    // Plain English Summary: Position the vocal Candlestick meter in the bottom-left quadrant of the component.
    vocalCandlestick.setBounds (getLocalBounds().removeFromLeft(getWidth() / 2).removeFromBottom(getHeight() / 2));

    // Plain English Summary: Position the audio setup component and start/stop button.
    audioSetupComp.setBounds(getLocalBounds().removeFromBottom(100));
    startAndStopButton.setBounds(getLocalBounds().removeFromBottom(50).reduced(10));
}

void StandaloneVocalRecorderComponent::startOrStopAudio()
{
    if (getAudioDeviceManager().getCurrentAudioDevice() != nullptr)
    {
        shutdownAudio();
        startAndStopButton.setButtonText ("Start Audio");
    }
    else
    {
        if (startAudio())
        {
            startAndStopButton.setButtonText ("Stop Audio");
        }
    }
}

// Plain English Summary: Establishes a WebSocket connection to the backend server.
// This connection is used to send real-time vocal DSP analysis data for AI feedback.
void StandaloneVocalRecorderComponent::connectWebSocket()
{
    // TODO: Get WebSocket URL from a configuration or processor parameter
    // For now, hardcode to the local backend. In a real scenario, this would be dynamic.
    juce::URL wsUrl("ws://localhost:3000/ws/vst"); 

    webSocket = std::make_unique<juce::WebSockets::WebSocket>();
    if (webSocket->connect(wsUrl))
    {
        DBG("Standalone WebSocket connected successfully!");
    }
    else
    {
        DBG("Standalone WebSocket connection failed!");
    }
}

// Plain English Summary: Closes the active WebSocket connection.
void StandaloneVocalRecorderComponent::disconnectWebSocket()
{
    if (webSocket && webSocket->isConnected())
    {
        webSocket->disconnect();
        DBG("Standalone WebSocket disconnected.");
    }
    webSocket = nullptr;
}

// Plain English Summary: Sends vocal DSP analysis metrics to the backend via WebSocket.
// This data is used by the Aifred AI Brain for context-aware feedback.
void StandaloneVocalRecorderComponent::sendVocalDspMetrics(const aifred::dsp::AnalysisFrame& frame)
{
    if (webSocket && webSocket->isConnected())
    {
        juce::DynamicObject::Ptr dspData = new juce::DynamicObject();
        dspData->setProperty("loudness", frame.loudness);
        dspData->setProperty("dynamics", frame.dynamics);
        dspData->setProperty("tone", frame.tone);
        dspData->setProperty("stereo", frame.stereo);
        dspData->setProperty("transient", frame.transient);
        dspData->setProperty("bpm", frame.bpm);
        dspData->setProperty("summary", frame.summary);

        juce::var message = new juce::DynamicObject();
        message->setProperty("type", "DSP_UPDATE");
        message->setProperty("metrics", dspData);

        webSocket->sendTextMessage(juce::JSON::toString(message));
    }
}

//==============================================================================
StandaloneVocalRecorderApplication::StandaloneVocalRecorderApplication()
{
    // Plain English Summary: Set the custom look and feel for the application using the defined theme.
    laf.setTheme(Theme::getDarkTheme());
    setLookAndFeel(&laf);
}

void StandaloneVocalRecorderApplication::initialise (const juce::String& commandLine)
{
    // Plain English Summary: Create and show the main application window.
    mainWindow.reset (new MainWindow (getApplicationName()));
}

void StandaloneVocalRecorderApplication::shutdown()
{
    // Plain English Summary: Clear the main application window when shutting down.
    mainWindow = nullptr;
    // Plain English Summary: Reset the look and feel to default when shutting down.
    setLookAndFeel(nullptr);
}

void StandaloneVocalRecorderApplication::systemRequestedQuit()
{
    // Plain English Summary: Quit the application when a system quit is requested.
    quit();
}

//==============================================================================
StandaloneVocalRecorderApplication::MainWindow::MainWindow (const juce::String& name)
    : DocumentWindow (name, Theme::instance().bg, DocumentWindow::allButtons)
{
    // Plain English Summary: Set the content of the main window to the StandaloneVocalRecorderComponent.
    setContentOwned (new StandaloneVocalRecorderComponent(), true);

    // Plain English Summary: Center the window on the screen and make it visible.
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

void StandaloneVocalRecorderApplication::MainWindow::closeButtonPressed()
{
    // Plain English Summary: When the close button is pressed, inform the application to quit.
    JUCEApplication::getInstance()->systemRequestedQuit();
}
