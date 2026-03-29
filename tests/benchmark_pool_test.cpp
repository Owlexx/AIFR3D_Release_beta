#include "dawai/reference_engine/pools/benchmark_pool.hpp"
#include "dawai/reference_engine/profile_cache.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <fstream>

TEST_CASE("Benchmark pool aggregate computes means and bands", "[pools]")
{
    std::filesystem::create_directories("/tmp/dawai_pool");

    nlohmann::json pool = {
        {"name", "HipHop Pool"},
        {"genre", "Hip-Hop"},
        {"entries", nlohmann::json::array({
                        {{"profileId", "p1"}, {"profilePath", "/tmp/dawai_pool/p1.json"}},
                        {{"profileId", "p2"}, {"profilePath", "/tmp/dawai_pool/p2.json"}},
                    })},
    };

    {
        std::ofstream out("/tmp/dawai_pool/pool.json");
        out << pool.dump(2);
    }

    nlohmann::json profile = nlohmann::json::array({{
        {"id", "p1"},
        {"title", "A"},
        {"sourcePath", "A.wav"},
        {"spectrumBandsDb", {-30, -20}},
        {"integratedLufs", -9.0},
        {"shortTermLufsDistribution", {-8.0}},
        {"crestFactorDistribution", {7.0}},
        {"stereoWidth", 1.1},
        {"transientDensity", 0.02},
    }});
    {
        std::ofstream out("/tmp/dawai_pool/p1.json");
        out << profile.dump(2);
    }
    {
        std::ofstream out("/tmp/dawai_pool/p2.json");
        out << profile.dump(2);
    }

    REQUIRE(dawai::reference_engine::pools::buildReferenceCache("/tmp/dawai_pool/pool.json",
                                                                "/tmp/dawai_pool/cache.json"));

    dawai::reference_engine::ProfileCache cache;
    const auto profiles = cache.load("/tmp/dawai_pool/cache.json");
    const auto aggregate = dawai::reference_engine::pools::computePoolAggregate(profiles);

    REQUIRE(aggregate.integratedLufs.mean == Catch::Approx(-9.0));
    REQUIRE(aggregate.spectrumBands.size() == 2);
}
