#pragma once

#include <JuceHeader.h>

namespace audiosynth::parameters
{

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputTrim", 1}, "Input Trim", -12.0f, 12.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"demoMode", 1},
                                                                "Demo Mode", false));

    return {params.begin(), params.end()};
}

} // namespace audiosynth::parameters
