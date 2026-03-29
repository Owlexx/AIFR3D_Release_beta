#include "dawai/audio_engine/session_serializer.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Session save/load roundtrip", "[session]")
{
    dawai::audio_engine::SessionState state;
    state.sessionName = "TestSession";
    state.masterFaderDb = -1.5F;

    dawai::audio_engine::TrackState track;
    track.name = "Track 1";
    track.solo = true;
    track.inserts.push_back({"plugin.id", "Plugin", false, {0xAA, 0xBB}});
    state.tracks.push_back(track);

    dawai::audio_engine::SessionSerializer serializer;
    const auto path = std::filesystem::path("/tmp/dawai_session_test.json");

    REQUIRE(serializer.save(path, state));

    const auto loaded = serializer.load(path);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->sessionName == "TestSession");
    REQUIRE(loaded->tracks.size() == 1);
    REQUIRE(loaded->tracks[0].inserts.size() == 1);
    REQUIRE(loaded->tracks[0].inserts[0].stateBlob.size() == 2);
}
