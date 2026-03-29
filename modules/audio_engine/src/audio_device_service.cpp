#include "dawai/audio_engine/audio_device_service.hpp"

#include <chrono>
#include <iostream>

namespace dawai::audio_engine
{

AudioDeviceService::AudioDeviceService() = default;

AudioDeviceService::~AudioDeviceService()
{
    m_manager.closeAudioDevice();
}

bool AudioDeviceService::initialise()
{
    const auto error = m_manager.initialiseWithDefaultDevices(0, 2);
    if (error.isNotEmpty())
    {
        std::cerr << "Audio init error: " << error << '\n';
        return false;
    }

    std::cout << "Audio device opened\n";
    return true;
}

bool AudioDeviceService::applyConfig(const DeviceConfig& config)
{
    if (auto* device = m_manager.getCurrentAudioDevice())
    {
        juce::AudioDeviceManager::AudioDeviceSetup setup;
        m_manager.getAudioDeviceSetup(setup);
        setup.sampleRate = config.sampleRate;
        setup.bufferSize = config.bufferSize;
        const auto error = m_manager.setAudioDeviceSetup(setup, true);
        if (error.isNotEmpty())
        {
            std::cerr << "Audio config error: " << error << '\n';
            return false;
        }

        std::cout << "Audio reconfigured sr=" << setup.sampleRate << " buffer=" << setup.bufferSize
                  << '\n';
        return true;
    }
    return false;
}

DeviceConfig AudioDeviceService::currentConfig() const
{
    DeviceConfig cfg;
    if (auto* device = m_manager.getCurrentAudioDevice())
    {
        cfg.sampleRate = device->getCurrentSampleRate();
        cfg.bufferSize = device->getCurrentBufferSizeSamples();
    }
    return cfg;
}

void AudioDeviceService::attachCallback(juce::AudioIODeviceCallback* callback)
{
    m_manager.addAudioCallback(callback);
}

void AudioDeviceService::detachCallback(juce::AudioIODeviceCallback* callback)
{
    m_manager.removeAudioCallback(callback);
}

void AudioDeviceService::noteCallbackTick()
{
    const auto now = std::chrono::steady_clock::now();
    const auto cfg = currentConfig();
    if (cfg.sampleRate <= 0.0)
    {
        return;
    }

    const auto expectedMs = static_cast<double>(cfg.bufferSize) / cfg.sampleRate * 1000.0;

    if (m_lastTick.has_value())
    {
        const auto elapsedMs = std::chrono::duration<double, std::milli>(now - *m_lastTick).count();
        if (elapsedMs > expectedMs * 2.2)
        {
            ++m_xruns;
            std::cerr << "Potential xrun detected (elapsed ms=" << elapsedMs << ")\n";
        }
    }

    m_lastTick = now;
}

} // namespace dawai::audio_engine
