#include "dawai/audio_engine/transport_controller.hpp"

namespace dawai::audio_engine
{

void TransportController::play()
{
    m_state.store(TransportState::Playing);
}

void TransportController::stop()
{
    m_state.store(TransportState::Stopped);
    m_currentSeconds.store(0.0);
}

void TransportController::pause()
{
    if (m_state.load() == TransportState::Playing)
    {
        m_state.store(TransportState::Paused);
    }
}

TransportState TransportController::state() const noexcept
{
    return m_state.load();
}

double TransportController::currentSeconds() const noexcept
{
    return m_currentSeconds.load();
}

void TransportController::advance(int numSamples, double sampleRate)
{
    if (m_state.load() != TransportState::Playing || sampleRate <= 0.0)
    {
        return;
    }

    const double delta = static_cast<double>(numSamples) / sampleRate;
    m_currentSeconds.store(m_currentSeconds.load() + delta);
}

} // namespace dawai::audio_engine
