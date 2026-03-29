#pragma once

#include <atomic>

namespace dawai::audio_engine
{

enum class TransportState
{
    Stopped,
    Playing,
    Paused
};

class TransportController
{
  public:
    void play();
    void stop();
    void pause();

    [[nodiscard]] TransportState state() const noexcept;
    [[nodiscard]] double currentSeconds() const noexcept;

    void advance(int numSamples, double sampleRate);

  private:
    std::atomic<TransportState> m_state{TransportState::Stopped};
    std::atomic<double> m_currentSeconds{0.0};
};

} // namespace dawai::audio_engine
