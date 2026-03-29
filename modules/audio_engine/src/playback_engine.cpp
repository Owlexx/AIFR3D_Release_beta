#include "dawai/audio_engine/playback_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

namespace dawai::audio_engine
{

namespace
{
float clampPan(float pan)
{
    return std::clamp(pan, -1.0F, 1.0F);
}

thread_local bool g_inRealtimeAudioCallback = false;

struct RealtimeAudioScope
{
    RealtimeAudioScope()
    {
        g_inRealtimeAudioCallback = true;
    }
    ~RealtimeAudioScope()
    {
        g_inRealtimeAudioCallback = false;
    }
};

void assertNotRealtimeThread()
{
    // Guard against introducing blocking operations on the realtime callback thread.
    jassert(!g_inRealtimeAudioCallback);
}

} // namespace

PlaybackEngine::PlaybackEngine(std::size_t trackCount) : m_meteringEngine(1024, 32)
{
    if (trackCount > kMaxTracks)
    {
        trackCount = kMaxTracks;
    }
    m_tracks.resize(trackCount);
    m_trackRacks.resize(trackCount);
    for (std::size_t i = 0; i < m_tracks.size(); ++i)
    {
        m_tracks[i].state.name = "Track " + std::to_string(i + 1);
        m_trackMuteRt[i].store(false, std::memory_order_relaxed);
        m_trackSoloRt[i].store(false, std::memory_order_relaxed);
        m_trackFaderDbRt[i].store(0.0F, std::memory_order_relaxed);
        m_trackPanRt[i].store(0.0F, std::memory_order_relaxed);
    }
    m_trackProcessBuffer.setSize(2, kRtMaxSamples, false, false, true);
    m_formatManager.registerBasicFormats();
    m_analysisWorker = std::thread([this] { analysisLoop(); });
}

PlaybackEngine::~PlaybackEngine()
{
    m_deviceService.detachCallback(this);
    m_analysisStopping.store(true);
    if (m_analysisWorker.joinable())
    {
        m_analysisWorker.join();
    }
}

bool PlaybackEngine::initialiseAudio()
{
    if (!m_deviceService.initialise())
    {
        return false;
    }
    m_deviceService.attachCallback(this);
    return true;
}

bool PlaybackEngine::loadAudioToTrack(const std::filesystem::path& filePath, int trackIndex)
{
    assertNotRealtimeThread();
    if (m_transport.state() == TransportState::Playing)
    {
        return false;
    }

    std::scoped_lock lock(m_stateMutex);
    if (trackIndex < 0 || static_cast<std::size_t>(trackIndex) >= m_tracks.size())
    {
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(
        m_formatManager.createReaderFor(juce::File(filePath.string())));
    if (!reader)
    {
        return false;
    }

    juce::AudioBuffer<float> loadedBuffer(static_cast<int>(reader->numChannels),
                                          static_cast<int>(reader->lengthInSamples));

    if (!reader->read(&loadedBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true))
    {
        return false;
    }

    auto& track = m_tracks[static_cast<std::size_t>(trackIndex)];
    track.audio = std::move(loadedBuffer);
    track.readPosition = 0;

    track.state.clips.clear();
    track.state.clips.push_back(
        {filePath.string(), 0.0, reader->lengthInSamples / reader->sampleRate});

    return true;
}

void PlaybackEngine::play()
{
    m_transport.play();
}

void PlaybackEngine::stop()
{
    assertNotRealtimeThread();
    m_transport.stop();
    std::scoped_lock lock(m_stateMutex);
    for (auto& track : m_tracks)
    {
        track.readPosition = 0;
    }
}

void PlaybackEngine::pause()
{
    m_transport.pause();
}

SessionState PlaybackEngine::sessionState() const
{
    assertNotRealtimeThread();
    std::scoped_lock lock(m_stateMutex);
    SessionState session;
    session.masterFaderDb = m_masterFaderDb.load(std::memory_order_relaxed);
    for (const auto& track : m_tracks)
    {
        session.tracks.push_back(track.state);
    }
    return session;
}

void PlaybackEngine::setSessionState(const SessionState& state)
{
    assertNotRealtimeThread();
    if (m_transport.state() == TransportState::Playing)
    {
        m_transport.pause();
    }

    std::scoped_lock lock(m_stateMutex);
    m_masterFaderDb.store(state.masterFaderDb, std::memory_order_relaxed);

    const std::size_t limit = std::min(state.tracks.size(), m_tracks.size());
    for (std::size_t i = 0; i < limit; ++i)
    {
        m_tracks[i].state = state.tracks[i];
        m_trackMuteRt[i].store(state.tracks[i].mute, std::memory_order_relaxed);
        m_trackSoloRt[i].store(state.tracks[i].solo, std::memory_order_relaxed);
        m_trackFaderDbRt[i].store(state.tracks[i].faderDb, std::memory_order_relaxed);
        m_trackPanRt[i].store(clampPan(state.tracks[i].pan), std::memory_order_relaxed);
        for (std::size_t slot = 0; slot < state.tracks[i].inserts.size() &&
                                   slot < plugin_host::PluginRack::kSlots;
             ++slot)
        {
            m_trackRacks[i].setBypass(slot, state.tracks[i].inserts[slot].bypass);
        }
    }
}

void PlaybackEngine::setTrackMute(int trackIndex, bool mute)
{
    assertNotRealtimeThread();
    std::scoped_lock lock(m_stateMutex);
    if (trackIndex >= 0 && static_cast<std::size_t>(trackIndex) < m_tracks.size())
    {
        m_tracks[static_cast<std::size_t>(trackIndex)].state.mute = mute;
        m_trackMuteRt[static_cast<std::size_t>(trackIndex)].store(mute, std::memory_order_relaxed);
    }
}

void PlaybackEngine::setTrackSolo(int trackIndex, bool solo)
{
    assertNotRealtimeThread();
    std::scoped_lock lock(m_stateMutex);
    if (trackIndex >= 0 && static_cast<std::size_t>(trackIndex) < m_tracks.size())
    {
        m_tracks[static_cast<std::size_t>(trackIndex)].state.solo = solo;
        m_trackSoloRt[static_cast<std::size_t>(trackIndex)].store(solo, std::memory_order_relaxed);
    }
}

void PlaybackEngine::setTrackFaderDb(int trackIndex, float db)
{
    assertNotRealtimeThread();
    std::scoped_lock lock(m_stateMutex);
    if (trackIndex >= 0 && static_cast<std::size_t>(trackIndex) < m_tracks.size())
    {
        m_tracks[static_cast<std::size_t>(trackIndex)].state.faderDb = db;
        m_trackFaderDbRt[static_cast<std::size_t>(trackIndex)].store(db, std::memory_order_relaxed);
    }
}

void PlaybackEngine::setTrackPan(int trackIndex, float pan)
{
    assertNotRealtimeThread();
    std::scoped_lock lock(m_stateMutex);
    if (trackIndex >= 0 && static_cast<std::size_t>(trackIndex) < m_tracks.size())
    {
        const float clamped = clampPan(pan);
        m_tracks[static_cast<std::size_t>(trackIndex)].state.pan = clamped;
        m_trackPanRt[static_cast<std::size_t>(trackIndex)].store(clamped, std::memory_order_relaxed);
    }
}

bool PlaybackEngine::loadVst3ToTrackInsert(const std::filesystem::path& pluginPath, int trackIndex,
                                           int slotIndex, juce::String& error)
{
    assertNotRealtimeThread();
    if (m_transport.state() == TransportState::Playing)
    {
        error = "Stop transport before loading plugins";
        return false;
    }

    if (trackIndex < 0 || static_cast<std::size_t>(trackIndex) >= m_tracks.size())
    {
        error = "Invalid track index";
        return false;
    }
    if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= plugin_host::PluginRack::kSlots)
    {
        error = "Invalid insert slot";
        return false;
    }
    if (!std::filesystem::exists(pluginPath))
    {
        error = "Plugin path not found";
        return false;
    }

    const auto searchRoot = std::filesystem::path(pluginPath).parent_path();
    const auto discovered = m_pluginScanner.scanVST3({searchRoot});
    const auto targetName = std::filesystem::path(pluginPath).filename().string();

    const juce::PluginDescription* chosen = nullptr;
    for (const auto& desc : discovered)
    {
        const auto candidatePath = std::filesystem::path(desc.fileOrIdentifier.toStdString());
        if (!candidatePath.empty() && candidatePath.filename() == targetName)
        {
            chosen = &desc;
            break;
        }
    }
    if (chosen == nullptr)
    {
        error = "VST3 not discovered in selected path";
        return false;
    }

    auto instance = plugin_host::PluginInstance::create(
        *chosen, m_pluginScanner.formatManager(), m_rtSampleRate, m_rtBlockSize, error);
    if (!instance)
    {
        if (error.isEmpty())
        {
            error = "Plugin instance creation failed";
        }
        return false;
    }

    std::scoped_lock lock(m_stateMutex);
    auto& rack = m_trackRacks[static_cast<std::size_t>(trackIndex)];
    if (!rack.setSlot(static_cast<std::size_t>(slotIndex), std::move(instance), *chosen))
    {
        error = "Failed to assign plugin slot";
        return false;
    }
    rack.setBypass(static_cast<std::size_t>(slotIndex), false);

    auto& trackState = m_tracks[static_cast<std::size_t>(trackIndex)].state;
    if (trackState.inserts.size() < plugin_host::PluginRack::kSlots)
    {
        trackState.inserts.resize(plugin_host::PluginRack::kSlots);
    }
    auto& insertState = trackState.inserts[static_cast<std::size_t>(slotIndex)];
    insertState.pluginId = chosen->fileOrIdentifier.toStdString();
    insertState.pluginName = chosen->name.toStdString();
    insertState.bypass = false;
    return true;
}

void PlaybackEngine::setTrackInsertBypass(int trackIndex, int slotIndex, bool bypass)
{
    assertNotRealtimeThread();
    std::scoped_lock lock(m_stateMutex);
    if (trackIndex < 0 || static_cast<std::size_t>(trackIndex) >= m_tracks.size())
    {
        return;
    }
    if (slotIndex < 0 || static_cast<std::size_t>(slotIndex) >= plugin_host::PluginRack::kSlots)
    {
        return;
    }

    auto& trackState = m_tracks[static_cast<std::size_t>(trackIndex)].state;
    if (trackState.inserts.size() < plugin_host::PluginRack::kSlots)
    {
        trackState.inserts.resize(plugin_host::PluginRack::kSlots);
    }
    trackState.inserts[static_cast<std::size_t>(slotIndex)].bypass = bypass;
    m_trackRacks[static_cast<std::size_t>(trackIndex)].setBypass(static_cast<std::size_t>(slotIndex),
                                                                 bypass);
}

float PlaybackEngine::dbToLinear(float db)
{
    return std::pow(10.0F, db / 20.0F);
}

void PlaybackEngine::audioDeviceIOCallbackWithContext(
    const float* const* /*inputChannelData*/, int /*numInputChannels*/,
    float* const* outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext& /*context*/)
{
    RealtimeAudioScope realtimeScope;
    ModuleProfiler::ScopedSample audioSample(m_profiler, "audio_callback");
    m_deviceService.noteCallbackTick();

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        std::fill(outputChannelData[channel], outputChannelData[channel] + numSamples, 0.0F);
    }

    const auto state = m_transport.state();
    if (state != TransportState::Playing)
    {
        return;
    }

    bool anySolo = false;
    for (std::size_t i = 0; i < m_tracks.size(); ++i)
    {
        anySolo = anySolo || m_trackSoloRt[i].load(std::memory_order_relaxed);
    }

    for (std::size_t t = 0; t < m_tracks.size(); ++t)
    {
        const bool isSolo = m_trackSoloRt[t].load(std::memory_order_relaxed);
        const bool isMuted = m_trackMuteRt[t].load(std::memory_order_relaxed);
        const bool isAudible = anySolo ? isSolo : !isMuted;
        if (!isAudible)
        {
            continue;
        }

        auto& track = m_tracks[t];
        if (track.audio.getNumSamples() == 0)
        {
            continue;
        }

        const float gain = dbToLinear(m_trackFaderDbRt[t].load(std::memory_order_relaxed));
        const float pan = clampPan(m_trackPanRt[t].load(std::memory_order_relaxed));
        const float leftPan = std::sqrt(0.5F * (1.0F - pan));
        const float rightPan = std::sqrt(0.5F * (1.0F + pan));
        const int processSamples = std::min(numSamples, kRtMaxSamples);
        m_trackProcessBuffer.clear();
        for (int i = 0; i < processSamples; ++i)
        {
            if (track.readPosition >= track.audio.getNumSamples())
            {
                break;
            }
            const float sampleL = track.audio.getNumChannels() > 0
                                      ? track.audio.getSample(0, track.readPosition)
                                      : 0.0F;
            const float sampleR = track.audio.getNumChannels() > 1
                                      ? track.audio.getSample(1, track.readPosition)
                                      : sampleL;
            m_trackProcessBuffer.setSample(0, i, sampleL);
            m_trackProcessBuffer.setSample(1, i, sampleR);
            ++track.readPosition;
        }

        m_pluginMidiBuffer.clear();
        if (t < m_trackRacks.size())
        {
            m_trackRacks[t].process(m_trackProcessBuffer, m_pluginMidiBuffer);
        }

        for (int i = 0; i < processSamples; ++i)
        {
            const float sampleL = m_trackProcessBuffer.getSample(0, i);
            const float sampleR = m_trackProcessBuffer.getSample(1, i);
            outputChannelData[0][i] += sampleL * gain * leftPan;
            if (numOutputChannels > 1)
            {
                outputChannelData[1][i] += sampleR * gain * rightPan;
            }
        }

        for (int i = processSamples; i < numSamples; ++i)
        {
            if (track.readPosition >= track.audio.getNumSamples())
            {
                break;
            }
            const float sampleL = track.audio.getNumChannels() > 0
                                      ? track.audio.getSample(0, track.readPosition)
                                      : 0.0F;
            const float sampleR = track.audio.getNumChannels() > 1
                                      ? track.audio.getSample(1, track.readPosition)
                                      : sampleL;
            outputChannelData[0][i] += sampleL * gain * leftPan;
            if (numOutputChannels > 1)
            {
                outputChannelData[1][i] += sampleR * gain * rightPan;
            }
            ++track.readPosition;
        }
    }

    const float master = dbToLinear(m_masterFaderDb.load(std::memory_order_relaxed));
    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            outputChannelData[channel][i] *= master;
        }
    }

    m_transport.advance(numSamples, m_deviceService.currentConfig().sampleRate);

    MeterTapBuffer tap;
    tap.numChannels = std::min(numOutputChannels, MeterTapBuffer::kMaxChannels);
    tap.numSamples = std::min(numSamples, MeterTapBuffer::kMaxSamples);
    tap.sampleRate = m_deviceService.currentConfig().sampleRate;

    for (int channel = 0; channel < tap.numChannels; ++channel)
    {
        for (int i = 0; i < tap.numSamples; ++i)
        {
            tap.data[static_cast<std::size_t>(channel)][static_cast<std::size_t>(i)] =
                outputChannelData[channel][i];
        }
    }

    (void)m_meterTapQueue.push(tap);
}

void PlaybackEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        m_rtSampleRate = device->getCurrentSampleRate();
        m_rtBlockSize = device->getCurrentBufferSizeSamples();
        std::cout << "Audio device start: " << device->getName() << '\n';
    }
}

void PlaybackEngine::audioDeviceStopped()
{
    std::cout << "Audio device stopped\n";
}

dawai::metering::MeterSnapshot PlaybackEngine::lastMeterSnapshot() const
{
    std::scoped_lock lock(m_meterMutex);
    return m_lastMeter;
}

void PlaybackEngine::analysisLoop()
{
    while (!m_analysisStopping.load())
    {
        MeterTapBuffer tap;
        if (!m_meterTapQueue.pop(tap))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        ModuleProfiler::ScopedSample analysisSample(m_profiler, "metering_worker");
        dawai::metering::AudioBlock block;
        block.channels.resize(static_cast<std::size_t>(tap.numChannels));

        for (int channel = 0; channel < tap.numChannels; ++channel)
        {
            block.channels[static_cast<std::size_t>(channel)].assign(
                tap.data[static_cast<std::size_t>(channel)].begin(),
                tap.data[static_cast<std::size_t>(channel)].begin() + tap.numSamples);
        }

        const auto snapshot = m_meteringEngine.process(block, tap.sampleRate);
        {
            std::scoped_lock meterLock(m_meterMutex);
            m_lastMeter = snapshot;
        }
    }
}

} // namespace dawai::audio_engine
