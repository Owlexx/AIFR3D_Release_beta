#include "dawai/aifr3d_core/analysis_writer.hpp"

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <fstream>

namespace dawai::aifr3d_core
{

namespace
{
bool shouldWriteAnalysisFiles()
{
    if (const char* env = std::getenv("DAWAI_WRITE_ANALYSIS_JSON"); env != nullptr)
    {
        return std::string(env) == "1";
    }
    return false;
}
} // namespace

std::string AnalysisWriter::buildResultJson(const FeatureSet& features,
                                            const ScoringResult& scoring) const
{
    nlohmann::json resultJson;
    resultJson["overall_rating"] = scoring.overallRating;
    resultJson["category_scores"] = nlohmann::json::array();

    for (const auto& category : scoring.categoryScores)
    {
        resultJson["category_scores"].push_back({
            {"category", category.category},
            {"score", category.score},
            {"signal_clarity", category.signalClarity},
            {"confidence", category.confidence},
            {"professional_average_range", {category.proBandLow, category.proBandHigh}},
        });
    }

    resultJson["comparison_mode"] = {{"live_reference", "nearest_track"},
                                       {"target_window", "genre_mean_plus_minus_3"}};
    resultJson["matched_reference"] = {{"id", features.referenceProfile.id},
                                         {"title", features.referenceProfile.title},
                                         {"source_path", features.referenceProfile.sourcePath}};
    resultJson["genre_target"] = {{"id", features.targetProfile.id},
                                    {"title", features.targetProfile.title},
                                    {"integrated_lufs", features.targetProfile.integratedLufs},
                                    {"true_peak_dbtp", features.targetProfile.truePeakDbtp},
                                    {"stereo_width", features.targetProfile.stereoWidth},
                                    {"transient_density", features.targetProfile.transientDensity},
                                    {"target_window", {-features.targetWindow, features.targetWindow}}};
    resultJson["lufs_delta"] = features.delta.integratedLufsDelta;
    resultJson["target_lufs_delta"] = features.targetDelta.integratedLufsDelta;
    resultJson["short_term_lufs"] = features.mixSnapshot.loudness.shortTermLufs;
    resultJson["rms_db"] = features.mixSnapshot.dynamics.rmsDb;
    resultJson["peak_dbfs"] = features.mixSnapshot.dynamics.peakDbfs;
    resultJson["crest_delta"] = features.delta.crestFactorDelta;
    resultJson["target_crest_delta"] = features.targetDelta.crestFactorDelta;
    resultJson["crest_factor_db"] = features.mixSnapshot.dynamics.crestFactorDb;
    resultJson["transient_density"] = features.mixSnapshot.dynamics.transientDensity;
    resultJson["width_delta"] = features.delta.widthDelta;
    resultJson["target_width_delta"] = features.targetDelta.widthDelta;
    resultJson["stereo_width"] = features.mixSnapshot.stereo.width;
    resultJson["stereo_correlation"] = features.mixSnapshot.stereo.correlation;
    resultJson["stereo_mid_energy"] = features.mixSnapshot.stereo.midEnergy;
    resultJson["stereo_side_energy"] = features.mixSnapshot.stereo.sideEnergy;
    resultJson["phase_risk"] = features.mixSnapshot.stereo.phaseRisk;
    resultJson["sub_mono_integrity"] = features.mixSnapshot.stereo.subMonoIntegrity;
    resultJson["true_peak_dbtp"] = features.mixSnapshot.loudness.truePeakDbtp;
    resultJson["true_peak_reference_dbtp"] = features.referenceProfile.truePeakDbtp;
    resultJson["true_peak_vs_reference_db"] = features.delta.truePeakDelta;
    resultJson["true_peak_target_dbtp"] = features.targetProfile.truePeakDbtp;
    resultJson["true_peak_vs_target_db"] = features.targetDelta.truePeakDelta;
    resultJson["spectrum"] = {{"low_band_db", features.mixSnapshot.spectrum.lowBandDb},
                                {"low_mid_band_db", features.mixSnapshot.spectrum.lowMidBandDb},
                                {"presence_band_db", features.mixSnapshot.spectrum.presenceBandDb},
                                {"air_band_db", features.mixSnapshot.spectrum.airBandDb},
                                {"spectral_tilt_db", features.mixSnapshot.spectrum.spectralTiltDb}};

    return resultJson.dump(2);
}

std::string AnalysisWriter::buildFixListJson(const FeatureSet& features,
                                             const ScoringResult& scoring,
                                             const std::vector<FixListItem>& fixList) const
{
    (void)features;
    (void)scoring;
    nlohmann::json fixJson = nlohmann::json::array();
    for (const auto& item : fixList)
    {
        fixJson.push_back({
            {"priority", item.priority},
            {"category", item.category},
            {"what_changed", item.whatChanged},
            {"where_you_stand", item.whereYouStand},
            {"what_to_do_next", item.whatToDoNext},
            {"options", item.options},
            {"signal_clarity", item.signalClarity},
            {"confidence", item.confidence},
            {"score", item.score},
            {"target_severity", item.targetSeverity},
            {"reference_severity", item.referenceSeverity},
            {"target_delta", item.targetDelta},
            {"reference_delta", item.referenceDelta},
            {"target_window", {-features.targetWindow, features.targetWindow}},
        });
    }

    return fixJson.dump(2);
}

bool AnalysisWriter::write(const std::filesystem::path& analysisDir, const FeatureSet& features,
                           const ScoringResult& scoring,
                           const std::vector<FixListItem>& fixList) const
{
    if (!shouldWriteAnalysisFiles())
    {
        return true;
    }

    std::filesystem::create_directories(analysisDir);

    std::ofstream resultOut(analysisDir / "result.json");
    std::ofstream fixOut(analysisDir / "fix_list.json");

    if (!resultOut || !fixOut)
    {
        return false;
    }

    resultOut << buildResultJson(features, scoring);
    fixOut << buildFixListJson(features, scoring, fixList);
    return true;
}

} // namespace dawai::aifr3d_core
