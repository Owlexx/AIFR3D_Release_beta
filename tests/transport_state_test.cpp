#include "dawai/audio_engine/transport_controller.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Transport state transitions play-stop", "[transport]")
{
    dawai::audio_engine::TransportController transport;

    REQUIRE(transport.state() == dawai::audio_engine::TransportState::Stopped);

    transport.play();
    REQUIRE(transport.state() == dawai::audio_engine::TransportState::Playing);

    transport.advance(48000, 48000.0);
    REQUIRE(transport.currentSeconds() == Catch::Approx(1.0).epsilon(0.001));

    transport.stop();
    REQUIRE(transport.state() == dawai::audio_engine::TransportState::Stopped);
    REQUIRE(transport.currentSeconds() == Catch::Approx(0.0).margin(1.0e-6));
}
