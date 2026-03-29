#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace dawai::advisory_layer
{

struct SessionContext
{
    std::string genre;
    std::string userIntent;
    std::string constraints;
    std::string conversationId = "default";
    std::string prompt;
    std::string model;
    std::vector<std::filesystem::path> attachments;
    std::string liveAnalysisJson;
    std::string liveFixCandidatesJson;
};

struct AdvisoryIssue
{
    std::string issueType;
    std::string severity;
    std::string summary;
    std::string whyItMatters;
    std::vector<std::string> tryFirst;
    std::vector<std::string> ignoreForNow;
    std::vector<std::string> evidencePointers;
};

struct AdvisoryActionSuggestion
{
    std::string id;
    std::string label;
    bool destructive = false;
    bool requiresSave = false;
};

struct AdvisoryOutput
{
    std::string summary30s;
    std::vector<std::string> top3Fixes;
    std::vector<AdvisoryIssue> issues;
    std::vector<AdvisoryActionSuggestion> actionSuggestions;
    std::string safePath;
    std::string boldPath;
    double signalClarity = 0.0;
    // Deprecated compatibility alias.
    double confidence = 0.0;
    std::string details;
};

class IAdviser
{
  public:
    virtual ~IAdviser() = default;
    virtual std::optional<AdvisoryOutput> advise(const std::filesystem::path& analysisDir,
                                                 const SessionContext& context) = 0;
};

class AnalysisSandbox
{
  public:
    explicit AnalysisSandbox(std::filesystem::path baseDir = "analysis",
                             bool allowExternalWrites = false)
        : m_baseDir(std::move(baseDir)), m_allowExternalWrites(allowExternalWrites)
    {
    }

    [[nodiscard]] bool canWrite(const std::filesystem::path& path) const;

  private:
    std::filesystem::path m_baseDir;
    bool m_allowExternalWrites = false;
};

using AdvisoryCallback = std::function<void(std::optional<AdvisoryOutput>)>;

} // namespace dawai::advisory_layer
