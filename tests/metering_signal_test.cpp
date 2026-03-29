#include "dawai/metering/metering_engine.hpp"

#include "test_utils.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Metering metrics for sine and noise within expected ranges", "[metering]")
{
    dawai::metering::MeteringEngine engine;

    const auto sine = test_utils::makeSine(1000.0, 48000.0, 48000, 0.5F);
    const auto sineMetrics = engine.process(sine, 48000.0);

    REQUIRE(sineMetrics.loudness.integratedLufs < -5.0);
    REQUIRE(sineMetrics.loudness.integratedLufs > -20.0);
    REQUIRE(sineMetrics.dynamics.crestFactorDb > 2.0);
    REQUIRE(sineMetrics.stereo.correlation == Catch::Approx(1.0).margin(0.05));

    const auto noise = test_utils::makeNoise(48000, 0.2F);
    const auto noiseMetrics = engine.process(noise, 48000.0);

    REQUIRE(noiseMetrics.dynamics.crestFactorDb < sineMetrics.dynamics.crestFactorDb + 6.0);
    REQUIRE(noiseMetrics.spectrum.averagedBinsDb.size() >= 8);
}
