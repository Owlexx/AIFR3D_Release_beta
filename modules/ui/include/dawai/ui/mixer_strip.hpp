#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>

namespace dawai::ui
{

class MixerStrip : public juce::Component
{
  public:
    explicit MixerStrip(int trackIndex);

    void resized() override;
    void paint(juce::Graphics& g) override;

    void setTrackName(const juce::String& name);

    std::function<void(int, bool)> onMute;
    std::function<void(int, bool)> onSolo;
    std::function<void(int, float)> onFader;
    std::function<void(int, float)> onPan;
    std::function<void(int, int, bool)> onInsertBypass;
    std::function<void(int, int)> onInsertLoad;

  private:
    int m_trackIndex;

    juce::Label m_name;
    juce::ToggleButton m_mute{"M"};
    juce::ToggleButton m_solo{"S"};
    juce::Slider m_fader;
    juce::Slider m_pan;
    std::array<juce::TextButton, 4> m_insertLoad{
        juce::TextButton("L1"), juce::TextButton("L2"), juce::TextButton("L3"),
        juce::TextButton("L4")};
    std::array<juce::ToggleButton, 4> m_insertBypass{
        juce::ToggleButton("I1"), juce::ToggleButton("I2"), juce::ToggleButton("I3"),
        juce::ToggleButton("I4")};
};

} // namespace dawai::ui
