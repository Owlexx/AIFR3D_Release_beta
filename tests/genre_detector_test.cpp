#include "dawai/reference_engine/genre_detector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>

TEST_CASE("Genre detector keeps nearest track match and genre mean target", "[reference]")
{
    const auto tempRoot = std::filesystem::temp_directory_path() / "dawai_genre_detector_test";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "profiles");

    const auto writeProfile = [&](const std::string& fileName, const std::string& genre,
                                  const std::string& id, double lufs, double truePeak,
                                  double crest, double width)
    {
        nlohmann::json payload = {
            {"id", id},
            {"title", id},
            {"sourcePath", fileName},
            {"genre", genre},
            {"integratedLufs", lufs},
            {"truePeakDbtp", truePeak},
            {"crestFactorDistribution", nlohmann::json::array({crest})},
            {"stereoWidth", width},
            {"transientDensity", 0.5},
            {"spectrumBandsDb", nlohmann::json::array({-30.0, -20.0, -15.0, -12.0})},
        };
        std::ofstream out(tempRoot / "profiles" / fileName);
        out << nlohmann::json::array({payload}).dump(2);
    };

    writeProfile("rap_a.json", "rap", "rap-a", -9.0, -0.8, 6.0, 0.35);
    writeProfile("rap_b.json", "rap", "rap-b", -6.0, -0.2, 9.0, 0.55);
    writeProfile("pop_a.json", "pop", "pop-a", -13.0, -1.4, 7.5, 0.75);

    nlohmann::json manifest = {
        {"references",
         nlohmann::json::array({
             {{"genre", "rap"}, {"profile_path", "profiles/rap_a.json"}},
             {{"genre", "rap"}, {"profile_path", "profiles/rap_b.json"}},
             {{"genre", "pop"}, {"profile_path", "profiles/pop_a.json"}},
         })},
    };
    {
        std::ofstream manifestOut(tempRoot / "pool_manifest.json");
        manifestOut << manifest.dump(2);
    }

    dawai::reference_engine::GenreDetector detector;
    REQUIRE(detector.load(tempRoot / "pool_manifest.json", tempRoot));

    dawai::metering::MeterSnapshot snapshot;
    snapshot.spectrum.averagedBinsDb = {-30.0, -20.0, -15.0, -12.0};
    snapshot.loudness.integratedLufs = -6.1;
    snapshot.loudness.truePeakDbtp = -0.25;
    snapshot.dynamics.crestFactorDb = 8.9;
    snapshot.stereo.width = 0.54;

    const auto detection = detector.detect(snapshot);
    REQUIRE(detection.has_value());
    REQUIRE(detection->genre == "rap");
    REQUIRE(detection->referenceMatch.id == "rap-b");
    REQUIRE(detection->referenceMean.id == "genre-mean-rap");
    REQUIRE(std::abs(detection->referenceMean.integratedLufs - (-7.5)) < 0.0001);
    REQUIRE(std::abs(detection->referenceMean.truePeakDbtp - (-0.5)) < 0.0001);

    std::filesystem::remove_all(tempRoot);
}
