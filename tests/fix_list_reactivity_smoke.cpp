#include "dawai/aifr3d_core/feature_ingestion.hpp"
#include "dawai/aifr3d_core/fix_list.hpp"
#include "dawai/aifr3d_core/scoring.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace
{
dawai::metering::MeterSnapshot makeSnapshot()
{
    dawai::metering::MeterSnapshot snapshot;
    snapshot.spectrum.averagedBinsDb = {-30.0, -29.0, -24.0, -23.5, -22.0, -21.5, -24.5, -26.0};
    snapshot.loudness.integratedLufs = -9.0;
    snapshot.loudness.shortTermLufs = -8.5;
    snapshot.loudness.truePeakDbtp = -1.0;
    snapshot.stereo.width = 0.95;
    snapshot.stereo.correlation = 0.18;
    snapshot.stereo.midEnergy = 0.78;
    snapshot.stereo.sideEnergy = 0.61;
    snapshot.stereo.phaseRisk = 0.08;
    snapshot.stereo.subMonoIntegrity = 0.96;
    snapshot.dynamics.rmsDb = -12.0;
    snapshot.dynamics.peakDbfs = -1.0;
    snapshot.dynamics.crestFactorDb = 11.0;
    snapshot.dynamics.transientDensity = 0.36;
    return snapshot;
}

dawai::reference_engine::ReferenceProfile makeReference(
    const dawai::metering::MeterSnapshot& snapshot)
{
    dawai::reference_engine::ReferenceProfile profile;
    profile.id = "genre-ref";
    profile.title = "Genre Reference";
    profile.spectrumBandsDb = snapshot.spectrum.averagedBinsDb;
    profile.integratedLufs = snapshot.loudness.integratedLufs;
    profile.truePeakDbtp = snapshot.loudness.truePeakDbtp;
    profile.crestFactorDistribution = {snapshot.dynamics.crestFactorDb};
    profile.stereoWidth = snapshot.stereo.width;
    profile.transientDensity = snapshot.dynamics.transientDensity;
    return profile;
}

struct ScenarioResult
{
    double overallRating = 0.0;
    std::string category;
    double targetDelta = 0.0;
};

ScenarioResult runScenario(const dawai::metering::MeterSnapshot& snapshot,
                           const dawai::reference_engine::ReferenceProfile& reference,
                           const dawai::reference_engine::ReferenceProfile& target)
{
    dawai::aifr3d_core::FeatureIngestor ingestor;
    const auto features = ingestor.ingest(snapshot, reference, target);

    dawai::aifr3d_core::ScoringEngine scoringEngine;
    const auto scoring = scoringEngine.score(features);

    dawai::aifr3d_core::FixListGenerator generator;
    const auto fixes = generator.generate(features, scoring);

    ScenarioResult out;
    out.overallRating = scoring.overallRating;
    if (!fixes.empty())
    {
        out.category = fixes.front().category;
        out.targetDelta = fixes.front().targetDelta;
    }
    return out;
}

bool require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}
} // namespace

int main()
{
    const auto balanced = makeSnapshot();
    const auto reference = makeReference(balanced);
    const auto target = makeReference(balanced);

    auto bassCut = balanced;
    bassCut.spectrum.averagedBinsDb[0] -= 14.0;
    bassCut.spectrum.averagedBinsDb[1] -= 12.0;

    auto lowMidBoost = balanced;
    lowMidBoost.spectrum.averagedBinsDb[2] += 8.5;
    lowMidBoost.spectrum.averagedBinsDb[3] += 9.0;

    auto highBoost = balanced;
    highBoost.spectrum.averagedBinsDb[6] += 8.0;
    highBoost.spectrum.averagedBinsDb[7] += 8.5;

    auto stereoCollapse = balanced;
    stereoCollapse.stereo.width = 0.08;
    stereoCollapse.stereo.correlation = 0.99;
    stereoCollapse.stereo.sideEnergy = 0.02;

    const auto balancedResult = runScenario(balanced, reference, target);
    const auto bassCutResult = runScenario(bassCut, reference, target);
    const auto lowMidBoostResult = runScenario(lowMidBoost, reference, target);
    const auto highBoostResult = runScenario(highBoost, reference, target);
    const auto stereoCollapseResult = runScenario(stereoCollapse, reference, target);

    std::cout << "balanced overall=" << balancedResult.overallRating
              << " top_fix=" << balancedResult.category
              << " delta=" << balancedResult.targetDelta << std::endl;
    std::cout << "bass_cut overall=" << bassCutResult.overallRating
              << " top_fix=" << bassCutResult.category
              << " delta=" << bassCutResult.targetDelta << std::endl;
    std::cout << "low_mid_boost overall=" << lowMidBoostResult.overallRating
              << " top_fix=" << lowMidBoostResult.category
              << " delta=" << lowMidBoostResult.targetDelta << std::endl;
    std::cout << "high_boost overall=" << highBoostResult.overallRating
              << " top_fix=" << highBoostResult.category
              << " delta=" << highBoostResult.targetDelta << std::endl;
    std::cout << "stereo_collapse overall=" << stereoCollapseResult.overallRating
              << " top_fix=" << stereoCollapseResult.category
              << " delta=" << stereoCollapseResult.targetDelta << std::endl;

    bool ok = true;
    ok &= require(bassCutResult.category == "Low End",
                  "bass cut did not move fix list to Low End");
    ok &= require(lowMidBoostResult.category == "Low-Mids",
                  "low-mid boost did not move fix list to Low-Mids");
    ok &= require(highBoostResult.category == "High End",
                  "high boost did not move fix list to High End");
    ok &= require(stereoCollapseResult.category == "Width/Depth",
                  "stereo collapse did not move fix list to Width/Depth");
    ok &= require(bassCutResult.overallRating < balancedResult.overallRating - 1.0,
                  "bass cut did not materially lower overall rating");
    ok &= require(lowMidBoostResult.overallRating < balancedResult.overallRating - 0.6,
                  "low-mid boost did not materially lower overall rating");
    ok &= require(highBoostResult.overallRating < balancedResult.overallRating - 0.6,
                  "high boost did not materially lower overall rating");
    ok &= require(std::abs(bassCutResult.targetDelta) > 5.0,
                  "bass cut delta did not materially change");
    ok &= require(std::abs(lowMidBoostResult.targetDelta) > 4.0,
                  "low-mid boost delta did not materially change");
    ok &= require(std::abs(highBoostResult.targetDelta) > 4.0,
                  "high boost delta did not materially change");

    return ok ? 0 : 1;
}
