#pragma once

#include <juce_audio_devices/juce_audio_devices.h>

#include <chrono>
#include <optional>
#include <string>

namespace dawai::audio_engine
{

struct DeviceConfig
{
    double sampleRate = 48000.0;
    int bufferSize = 512;
};

class AudioDeviceService
{
  public:
    AudioDeviceService();
    ~AudioDeviceService();

    bool initialise();
    bool applyConfig(const DeviceConfig& config);

    [[nodiscard]] DeviceConfig currentConfig() const;
    [[nodiscard]] juce::AudioDeviceManager& manager() noexcept
    {
        return m_manager;
    }

    void attachCallback(juce::AudioIODeviceCallback* callback);
    void detachCallback(juce::AudioIODeviceCallback* callback);

    void noteCallbackTick();
    [[nodiscard]] int xruns() const noexcept
    {
        return m_xruns;
    }

  private:
    juce::AudioDeviceManager m_manager;
    std::optional<std::chrono::steady_clock::time_point> m_lastTick;
    int m_xruns = 0;
};

} // namespace dawai::audio_engine
