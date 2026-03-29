#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dawai::audio_engine
{

struct ClipRegion
{
    std::string filePath;
    double startSeconds = 0.0;
    double lengthSeconds = 0.0;
};

struct PluginSlotState
{
    std::string pluginId;
    std::string pluginName;
    bool bypass = false;
    std::vector<std::uint8_t> stateBlob;
};

struct TrackState
{
    std::string name;
    bool mute = false;
    bool solo = false;
    float faderDb = 0.0F;
    float pan = 0.0F;
    std::vector<ClipRegion> clips;
    std::vector<PluginSlotState> inserts;
};

struct SessionState
{
    std::string sessionName = "Untitled";
    float masterFaderDb = 0.0F;
    std::vector<TrackState> tracks;
};

class MixerRules
{
  public:
    [[nodiscard]] static std::vector<bool>
    computeAudibleTracks(const std::vector<TrackState>& tracks);
};

} // namespace dawai::audio_engine
