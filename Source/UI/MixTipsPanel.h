#pragma once

#include <JuceHeader.h>

class MixTipsPanel : public juce::Component
{
  public:
    enum class Topic
    {
        FrequencyBalance = 0,
        IntegratedLoudness,
        DynamicRange,
        StereoWidth
    };

    MixTipsPanel();

    void paint(juce::Graphics&) override;
    void resized() override;

  private:
    void selectTopic(Topic topic);
    void updateButtonStates();
    void refreshContent();

    juce::TextButton frequencyButton{"Frequency Balance"};
    juce::TextButton loudnessButton{"Integrated Loudness"};
    juce::TextButton dynamicRangeButton{"Dynamic Range"};
    juce::TextButton stereoWidthButton{"Stereo Width"};
    juce::TextEditor content;
    Topic activeTopic = Topic::FrequencyBalance;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixTipsPanel)
};
