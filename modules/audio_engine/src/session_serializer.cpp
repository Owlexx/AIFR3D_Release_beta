#include "dawai/audio_engine/session_serializer.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace dawai::audio_engine
{

namespace
{
std::string bytesToHex(const std::vector<std::uint8_t>& bytes)
{
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (auto byte : bytes)
    {
        out.push_back(kHex[(byte >> 4U) & 0x0FU]);
        out.push_back(kHex[byte & 0x0FU]);
    }
    return out;
}

std::vector<std::uint8_t> hexToBytes(const std::string& hex)
{
    std::vector<std::uint8_t> out;
    if (hex.size() % 2 != 0)
    {
        return out;
    }

    auto parseNibble = [](char c) -> std::uint8_t
    {
        if (c >= '0' && c <= '9')
            return static_cast<std::uint8_t>(c - '0');
        if (c >= 'A' && c <= 'F')
            return static_cast<std::uint8_t>(10 + c - 'A');
        if (c >= 'a' && c <= 'f')
            return static_cast<std::uint8_t>(10 + c - 'a');
        return 0;
    };

    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2)
    {
        const auto hi = parseNibble(hex[i]);
        const auto lo = parseNibble(hex[i + 1]);
        out.push_back(static_cast<std::uint8_t>((hi << 4U) | lo));
    }
    return out;
}

} // namespace

bool SessionSerializer::save(const std::filesystem::path& path, const SessionState& state) const
{
    nlohmann::json root;
    root["sessionName"] = state.sessionName;
    root["masterFaderDb"] = state.masterFaderDb;
    root["tracks"] = nlohmann::json::array();

    for (const auto& track : state.tracks)
    {
        nlohmann::json t;
        t["name"] = track.name;
        t["mute"] = track.mute;
        t["solo"] = track.solo;
        t["faderDb"] = track.faderDb;
        t["pan"] = track.pan;

        t["clips"] = nlohmann::json::array();
        for (const auto& clip : track.clips)
        {
            t["clips"].push_back({
                {"filePath", clip.filePath},
                {"startSeconds", clip.startSeconds},
                {"lengthSeconds", clip.lengthSeconds},
            });
        }

        t["inserts"] = nlohmann::json::array();
        for (const auto& insert : track.inserts)
        {
            t["inserts"].push_back({
                {"pluginId", insert.pluginId},
                {"pluginName", insert.pluginName},
                {"bypass", insert.bypass},
                {"stateBlob", bytesToHex(insert.stateBlob)},
            });
        }

        root["tracks"].push_back(std::move(t));
    }

    std::ofstream output(path);
    if (!output)
    {
        return false;
    }

    output << root.dump(2);
    return true;
}

std::optional<SessionState> SessionSerializer::load(const std::filesystem::path& path) const
{
    std::ifstream input(path);
    if (!input)
    {
        return std::nullopt;
    }

    nlohmann::json root;
    input >> root;

    SessionState state;
    state.sessionName = root.value("sessionName", "Untitled");
    state.masterFaderDb = root.value("masterFaderDb", 0.0F);

    for (const auto& t : root.value("tracks", nlohmann::json::array()))
    {
        TrackState track;
        track.name = t.value("name", "Track");
        track.mute = t.value("mute", false);
        track.solo = t.value("solo", false);
        track.faderDb = t.value("faderDb", 0.0F);
        track.pan = t.value("pan", 0.0F);

        for (const auto& c : t.value("clips", nlohmann::json::array()))
        {
            track.clips.push_back({
                c.value("filePath", ""),
                c.value("startSeconds", 0.0),
                c.value("lengthSeconds", 0.0),
            });
        }

        for (const auto& i : t.value("inserts", nlohmann::json::array()))
        {
            track.inserts.push_back({
                i.value("pluginId", ""),
                i.value("pluginName", ""),
                i.value("bypass", false),
                hexToBytes(i.value("stateBlob", "")),
            });
        }

        state.tracks.push_back(std::move(track));
    }

    return state;
}

std::vector<bool> MixerRules::computeAudibleTracks(const std::vector<TrackState>& tracks)
{
    std::vector<bool> audible(tracks.size(), true);
    bool anySolo = false;
    for (const auto& track : tracks)
    {
        anySolo = anySolo || track.solo;
    }

    for (std::size_t i = 0; i < tracks.size(); ++i)
    {
        if (tracks[i].mute)
        {
            audible[i] = false;
            continue;
        }

        if (anySolo)
        {
            audible[i] = tracks[i].solo;
        }
    }

    return audible;
}

} // namespace dawai::audio_engine
