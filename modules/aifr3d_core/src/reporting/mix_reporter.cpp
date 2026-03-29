#include "dawai/aifr3d_core/reporting/mix_reporter.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace dawai::aifr3d_core::reporting
{

namespace
{
nlohmann::json toJson(const MixReport& report)
{
    nlohmann::json fix = nlohmann::json::array();
    for (const auto& item : report.fixList)
    {
        fix.push_back({
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
        });
    }

    return {
        {"timestamp_utc", report.timestampUtc},
        {"session_name", report.sessionName},
        {"reference_used", report.referenceUsed},
        {"rating", report.rating},
        {"fix_list", fix},
    };
}

void pruneReportHistory(const std::filesystem::path& folder, std::size_t keep)
{
    std::vector<std::filesystem::directory_entry> files;
    if (!std::filesystem::exists(folder))
    {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        if (entry.path().extension() == ".json")
        {
            files.push_back(entry);
        }
    }
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b)
              { return a.path().filename().string() > b.path().filename().string(); });
    if (files.size() <= keep)
    {
        return;
    }
    for (std::size_t i = keep; i < files.size(); ++i)
    {
        auto base = files[i].path();
        base.replace_extension("");
        std::error_code ec;
        std::filesystem::remove(files[i].path(), ec);
        std::filesystem::remove(base.string() + ".md", ec);
    }
}

MixReport fromJson(const nlohmann::json& j)
{
    MixReport report;
    report.timestampUtc = j.value("timestamp_utc", "");
    report.sessionName = j.value("session_name", "");
    report.referenceUsed = j.value("reference_used", "");
    report.rating = j.value("rating", 0.0);

    for (const auto& item : j.value("fix_list", nlohmann::json::array()))
    {
        report.fixList.push_back({
            item.value("priority", 0),
            item.value("category", ""),
            item.value("what_changed", ""),
            item.value("where_you_stand", ""),
            item.value("what_to_do_next", ""),
            item.value("options", std::vector<std::string>{}),
            item.value("signal_clarity", item.value("confidence", 0.0)),
            item.value("confidence", item.value("signal_clarity", 0.0)),
            item.value("score", 0.0),
            item.value("target_severity", 0.0),
            item.value("reference_severity", 0.0),
            item.value("target_delta", 0.0),
            item.value("reference_delta", 0.0),
        });
    }

    return report;
}

} // namespace

bool MixReporter::writeJsonAndMarkdown(const std::filesystem::path& folder,
                                       const MixReport& report) const
{
    std::filesystem::create_directories(folder);
    const auto base = folder / (report.sessionName + "_" + report.timestampUtc);

    std::ofstream jsonOut(base.string() + ".json");
    std::ofstream mdOut(base.string() + ".md");
    if (!jsonOut || !mdOut)
    {
        return false;
    }

    const auto json = toJson(report);
    jsonOut << json.dump(2);

    mdOut << "# Mix Report\n\n";
    mdOut << "- Timestamp: " << report.timestampUtc << "\n";
    mdOut << "- Session: " << report.sessionName << "\n";
    mdOut << "- Reference: " << report.referenceUsed << "\n";
    mdOut << "- Rating: " << report.rating << "\n\n";
    mdOut << "## Top Fixes\n";
    for (const auto& item : report.fixList)
    {
        mdOut << "- [" << item.priority << "] " << item.category << ": " << item.whatToDoNext
              << "\n";
    }

    pruneReportHistory(folder, 5);
    return true;
}

std::optional<MixReport> MixReporter::readJson(const std::filesystem::path& jsonPath) const
{
    std::ifstream input(jsonPath);
    if (!input)
    {
        return std::nullopt;
    }

    nlohmann::json j;
    input >> j;
    return fromJson(j);
}

std::vector<MixReport> MixReporter::loadLastN(const std::filesystem::path& folder,
                                              std::size_t count) const
{
    std::vector<std::filesystem::directory_entry> files;
    if (!std::filesystem::exists(folder))
    {
        return {};
    }

    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        if (entry.path().extension() == ".json")
        {
            files.push_back(entry);
        }
    }

    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b)
              { return a.path().filename().string() > b.path().filename().string(); });

    std::vector<MixReport> out;
    for (const auto& file : files)
    {
        if (out.size() >= count)
        {
            break;
        }
        if (const auto report = readJson(file.path()))
        {
            out.push_back(*report);
        }
    }
    return out;
}

ReportDelta compareReports(const MixReport& current, const MixReport& previous)
{
    return {
        current.rating - previous.rating,
        static_cast<int>(current.fixList.size()) - static_cast<int>(previous.fixList.size()),
    };
}

} // namespace dawai::aifr3d_core::reporting
