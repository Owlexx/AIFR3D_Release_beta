#pragma once

#include "dawai/audio_engine/audio_device_service.hpp"
#include "dawai/audio_engine/lockfree_ring.hpp"
#include "dawai/audio_engine/module_profiler.hpp"
#include "dawai/audio_engine/plugin_host/plugin_rack.hpp"
#include "dawai/audio_engine/plugin_host/scanner.hpp"
#include "dawai/audio_engine/track_model.hpp"
#include "dawai/audio_engine/transport_controller.hpp"
#include "dawai/metering/metering_engine.hpp"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>

namespace dawai::audio_engine
{

class PlaybackEngine : public juce::AudioIODeviceCallback
{
  public:
    explicit PlaybackEngine(std::size_t trackCount = 8);
    ~PlaybackEngine() override;

    bool initialiseAudio();

    bool loadAudioToTrack(const std::filesystem::path& filePath, int trackIndex);

    void play();
    void stop();
    void pause();

    [[nodiscard]] TransportState transportState() const noexcept
    {
        return m_transport.state();
    }
    [[nodiscard]] double transportSeconds() const noexcept
    {
        return m_transport.currentSeconds();
    }

    [[nodiscard]] SessionState sessionState() const;
    void setSessionState(const SessionState& state);

    void setTrackMute(int trackIndex, bool mute);
    void setTrackSolo(int trackIndex, bool solo);
    void setTrackFaderDb(int trackIndex, float db);
    void setTrackPan(int trackIndex, float pan);
    bool loadVst3ToTrackInsert(const std::filesystem::path& pluginPath, int trackIndex,
                               int slotIndex, juce::String& error);
    void setTrackInsertBypass(int trackIndex, int slotIndex, bool bypass);

    [[nodiscard]] dawai::metering::MeterSnapshot lastMeterSnapshot() const;
    [[nodiscard]] ModuleProfiler& profiler() noexcept
    {
        return m_profiler;
    }
    [[nodiscard]] AudioDeviceService& deviceService() noexcept
    {
        return m_deviceService;
    }

    // juce::AudioIODeviceCallback
    void
    audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                     float* const* outputChannelData, int numOutputChannels,
                                     int numSamples,
                                     const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

  private:
    struct RuntimeTrack
    {
        TrackState state;
        juce::AudioBuffer<float> audio;
        int readPosition = 0;
    };

    struct MeterTapBuffer
    {
        static constexpr int kMaxChannels = 2;
        static constexpr int kMaxSamples = 2048;

        int numChannels = 0;
        int numSamples = 0;
        double sampleRate = 48000.0;
        std::array<std::array<float, kMaxSamples>, kMaxChannels> data{};
    };

    static float dbToLinear(float db);
    void analysisLoop();

    AudioDeviceService m_deviceService;
    TransportController m_transport;
    juce::AudioFormatManager m_formatManager;

    static constexpr std::size_t kMaxTracks = 64;
    std::vector<RuntimeTrack> m_tracks;
    std::vector<plugin_host::PluginRack> m_trackRacks;
    plugin_host::Scanner m_pluginScanner;
    std::array<std::atomic<bool>, kMaxTracks> m_trackMuteRt{};
    std::array<std::atomic<bool>, kMaxTracks> m_trackSoloRt{};
    std::array<std::atomic<float>, kMaxTracks> m_trackFaderDbRt{};
    std::array<std::atomic<float>, kMaxTracks> m_trackPanRt{};
    std::atomic<float> m_masterFaderDb{0.0F};

    mutable std::mutex m_stateMutex;

    dawai::metering::MeteringEngine m_meteringEngine;
    mutable std::mutex m_meterMutex;
    dawai::metering::MeterSnapshot m_lastMeter;
    LockFreeRing<MeterTapBuffer, 16> m_meterTapQueue;
    std::atomic<bool> m_analysisStopping{false};
    std::thread m_analysisWorker;
    static constexpr int kRtMaxSamples = 8192;
    juce::AudioBuffer<float> m_trackProcessBuffer;
    juce::MidiBuffer m_pluginMidiBuffer;
    double m_rtSampleRate = 48000.0;
    int m_rtBlockSize = 512;

    ModuleProfiler m_profiler;
};

} // namespace dawai::audio_engine
