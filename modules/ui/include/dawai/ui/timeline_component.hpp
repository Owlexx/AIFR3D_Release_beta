#pragma once

#include "dawai/audio_engine/track_model.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

namespace dawai::ui
{

class TimelineComponent : public juce::Component
{
  public:
    void setSession(dawai::audio_engine::SessionState state);
    void setPlayheadSeconds(double seconds);

    void paint(juce::Graphics& g) override;

  private:
    dawai::audio_engine::SessionState m_session;
    double m_playheadSeconds = 0.0;
};

} // namespace dawai::ui
