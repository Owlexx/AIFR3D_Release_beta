#include "JsonReportWriter.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace aifred::dsp
{

bool JsonReportWriter::write(const std::filesystem::path& path,
                             const std::vector<AnalysisFrame>& frames) const
{
    constexpr std::size_t kMaxFramesWritten = 600;
    nlohmann::json root;
    root["schema_version"] = "1.0.0";
    root["timeline"] = nlohmann::json::array();

    const std::size_t start =
        (frames.size() > kMaxFramesWritten) ? (frames.size() - kMaxFramesWritten) : 0;
    for (std::size_t i = start; i < frames.size(); ++i)
    {
        const auto& f = frames[i];
        root["timeline"].push_back({{"t", f.tSec},
                                    {"behavior_index", f.behaviorIndex},
                                    {"mix_alignment", f.mixAlignment},
                                    {"signal_clarity", f.signalClarity},
                                    {"signal_stability", f.signalStability},
                                    {"life", f.lifeIndex},
                                    {"approval", f.approval},
                                    {"confidence", f.confidence},
                                    {"true_peak_reference_dbtp", f.referenceTruePeakDbTP},
                                    {"true_peak_target_dbtp", f.targetTruePeakDbTP},
                                    {"true_peak_vs_reference_db", f.truePeakVsReferenceDb},
                                    {"target_window_db", f.targetWindowDb},
                                    {"flags", f.flags}});
    }

    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path);
    if (!out)
        return false;

    out << root.dump(2);
    return true;
}

} // namespace aifred::dsp
