#include "dawai/reference_engine/ab_controller.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("A/B controller fades clicklessly", "[reference]")
{
    dawai::reference_engine::ABController ab(64);
    ab.setCompensationDb(-1.0);

    ab.setSource(dawai::reference_engine::ABController::Source::Reference);

    float previousMix = 1.0F;
    float previousRef = 0.0F;

    for (int i = 0; i < 64; ++i)
    {
        const auto [mixGain, refGain] = ab.nextGains();
        REQUIRE(std::abs(mixGain - previousMix) < 0.1F);
        REQUIRE(std::abs(refGain - previousRef) < 0.1F);
        previousMix = mixGain;
        previousRef = refGain;
    }
}
