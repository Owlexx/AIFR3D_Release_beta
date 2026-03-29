#include "dawai/audio_engine/track_model.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Solo logic mutes non-solo tracks", "[mixer]")
{
    std::vector<dawai::audio_engine::TrackState> tracks(3);
    tracks[0].solo = true;
    tracks[1].solo = false;
    tracks[2].solo = false;

    const auto audible = dawai::audio_engine::MixerRules::computeAudibleTracks(tracks);
    REQUIRE(audible[0]);
    REQUIRE_FALSE(audible[1]);
    REQUIRE_FALSE(audible[2]);
}
