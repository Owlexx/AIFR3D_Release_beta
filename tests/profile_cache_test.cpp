#include "dawai/reference_engine/profile_cache.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Profile cache save/load", "[reference]")
{
    dawai::reference_engine::ReferenceProfile profile;
    profile.id = "ref_1";
    profile.title = "Reference";
    profile.spectrumBandsDb = {-12.0, -8.0, -6.0};
    profile.spectralTiltDb = -4.5;
    profile.lowBandDb = -18.0;
    profile.lowMidBandDb = -14.0;
    profile.presenceBandDb = -10.0;
    profile.airBandDb = -12.0;
    profile.integratedLufs = -8.4;
    profile.shortTermLufs = -7.9;
    profile.truePeakDbtp = -0.6;
    profile.rmsDb = -11.2;
    profile.peakDbfs = -1.8;
    profile.crestFactorDb = 9.4;
    profile.correlation = 0.82;
    profile.stereoWidth = 0.34;
    profile.midEnergy = 0.76;
    profile.sideEnergy = 0.24;
    profile.phaseRisk = 0.08;
    profile.subMonoIntegrity = 0.94;
    profile.transientDensity = 0.28;
    profile.frequencyBalance.subEnergy = 0.08;
    profile.frequencyBalance.bassEnergy = 0.28;
    profile.frequencyBalance.lowMidEnergy = 0.18;
    profile.frequencyBalance.midEnergy = 0.22;
    profile.frequencyBalance.highMidEnergy = 0.16;
    profile.frequencyBalance.presenceEnergy = 0.06;
    profile.frequencyBalance.airEnergy = 0.02;

    dawai::reference_engine::ProfileCache cache;
    const auto path = std::filesystem::path("/tmp/dawai_profile_cache.json");

    REQUIRE(cache.save(path, {profile}));

    const auto loaded = cache.load(path);
    REQUIRE(loaded.size() == 1);
    REQUIRE(loaded[0].id == "ref_1");
    REQUIRE(loaded[0].integratedLufs == Catch::Approx(-8.4));
    REQUIRE(loaded[0].spectralTiltDb == Catch::Approx(-4.5));
    REQUIRE(loaded[0].truePeakDbtp == Catch::Approx(-0.6));
    REQUIRE(loaded[0].crestFactorDb == Catch::Approx(9.4));
    REQUIRE(loaded[0].correlation == Catch::Approx(0.82));
    REQUIRE(loaded[0].subMonoIntegrity == Catch::Approx(0.94));
    REQUIRE(loaded[0].frequencyBalance.bassEnergy == Catch::Approx(0.28));
    REQUIRE(loaded[0].frequencyBalance.highMidEnergy == Catch::Approx(0.16));
}
