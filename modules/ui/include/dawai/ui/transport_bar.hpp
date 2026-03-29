#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace dawai::ui
{

class TransportBar : public juce::Component
{
  public:
    TransportBar();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setTimeSeconds(double seconds);

    std::function<void()> onPlay;
    std::function<void()> onStop;
    std::function<void()> onPause;
    std::function<void()> onLoadTrack1;

  private:
    juce::TextButton m_playButton{"Play"};
    juce::TextButton m_stopButton{"Stop"};
    juce::TextButton m_pauseButton{"Pause"};
    juce::TextButton m_loadButton{"Load Audio to Track 1"};
    juce::Label m_timeLabel;
};

} // namespace dawai::ui
