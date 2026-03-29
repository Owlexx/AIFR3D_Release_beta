#include "dawai/advisory_layer/adviser.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Analysis sandbox restricts writes outside analysis root", "[advisory]")
{
    std::filesystem::create_directories("analysis");

    dawai::advisory_layer::AnalysisSandbox sandbox("analysis", false);
    REQUIRE(sandbox.canWrite("analysis/new_output.json"));
    REQUIRE_FALSE(sandbox.canWrite("/tmp/forbidden.json"));
}
