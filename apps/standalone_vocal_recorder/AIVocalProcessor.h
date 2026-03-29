/*
  ==============================================================================

    AIVocalProcessor.h
    Created: 29 Mar 2026 10:30:00pm
    Author: Manus

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../Source/Plugin/Theme.h"
#include "../../Source/DSP_v25/LoudnessEBUR128.h"
#include "../../Source/DSP_v25/FeatureExtractor.h"

//==============================================================================
/*
    This class handles the AI-driven vocal processing, including auto-tune and context-aware effects.
    It will analyze the vocal input and apply appropriate enhancements.
*/
class AIVocalProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AIVocalProcessor();
    ~AIVocalProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override; // Not used for standalone, but required by base class
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "AIVocalProcessor"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

private:
    // Plain English Summary: This analyzes the loudness of the vocal input.
    LoudnessEBUR128 loudnessAnalyzer;
    // Plain English Summary: This extracts key features from the vocal input for AI processing.
    FeatureExtractor featureExtractor;

    // Plain English Summary: This parameter controls the intensity of the auto-tune effect.
    juce::AudioParameterFloat* autoTuneAmount;
    // Plain English Summary: This parameter controls the intensity of the context-aware vocal effects.
    juce::AudioParameterFloat* fxAmount;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AIVocalProcessor)
};
