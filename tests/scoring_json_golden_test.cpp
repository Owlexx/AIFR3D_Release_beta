#include "dawai/aifr3d_core/analysis_writer.hpp"
#include "dawai/aifr3d_core/feature_ingestion.hpp"
#include "dawai/aifr3d_core/fix_list.hpp"
#include "dawai/aifr3d_core/scoring.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

TEST_CASE("Scoring output matches schema expectations", "[aifr3d]")
{
    dawai::metering::MeterSnapshot snapshot;
    snapshot.spectrum.averagedBinsDb = {-30, -20, -10, -12, -15, -18, -22, -25};
    snapshot.loudness.integratedLufs = -9.5;
    snapshot.dynamics.crestFactorDb = 7.1;
    snapshot.stereo.width = 0.9;

    dawai::reference_engine::ReferenceProfile reference;
    reference.id = "ref";
    reference.spectrumBandsDb = {-32, -21, -11, -13, -14, -17, -20, -24};
    reference.integratedLufs = -8.0;
    reference.crestFactorDistribution = {6.2};
    reference.stereoWidth = 1.0;

    dawai::reference_engine::ReferenceProfile target = reference;
    target.id = "genre-mean";
    target.integratedLufs = -9.0;

    dawai::aifr3d_core::FeatureIngestor ingestor;
    const auto features = ingestor.ingest(snapshot, reference, target);

    dawai::aifr3d_core::ScoringEngine scoringEngine;
    const auto scoring = scoringEngine.score(features);

    dawai::aifr3d_core::FixListGenerator fixGenerator;
    const auto fixList = fixGenerator.generate(features, scoring);

    dawai::aifr3d_core::AnalysisWriter writer;
    const auto resultJson = writer.buildResultJson(features, scoring);
    const auto fixJson = writer.buildFixListJson(features, scoring, fixList);

    nlohmann::json result = nlohmann::json::parse(resultJson);
    nlohmann::json fix = nlohmann::json::parse(fixJson);

    REQUIRE(result.contains("overall_rating"));
    REQUIRE(result.contains("category_scores"));
    REQUIRE(result.contains("comparison_mode"));
    REQUIRE(result.contains("matched_reference"));
    REQUIRE(result.contains("genre_target"));
    REQUIRE(fix.is_array());
    REQUIRE_FALSE(fix.empty());

    const auto goldenPath =
        std::filesystem::path(DAWAI_SOURCE_DIR) / "tests" / "golden" / "result_golden.json";
    std::ifstream goldenResultIn(goldenPath);
    REQUIRE(goldenResultIn.good());
    nlohmann::json golden;
    goldenResultIn >> golden;

    REQUIRE(result["category_scores"].size() == golden["category_scores"].size());
}
