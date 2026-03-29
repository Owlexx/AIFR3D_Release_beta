#include "dawai/advisory_layer/openai_adviser.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace dawai::advisory_layer
{

namespace
{
std::string shellEscapeSingleQuoted(std::string value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);
    escaped.push_back('\'');
    for (char c : value)
    {
        if (c == '\'')
        {
            escaped += "'\\''";
        }
        else
        {
            escaped.push_back(c);
        }
    }
    escaped.push_back('\'');
    return escaped;
}

bool commandAvailable(const char* cmd)
{
#if defined(_WIN32)
    const std::string probe = std::string("where ") + cmd + " >NUL 2>&1";
#else
    const std::string probe = std::string("command -v ") + cmd + " >/dev/null 2>&1";
#endif
    return std::system(probe.c_str()) == 0;
}

std::string runShellCommand(const std::string& command)
{
    std::array<char, 512> buffer{};
    std::string output;

#if defined(_WIN32)
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (pipe == nullptr)
    {
        return {};
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        output += buffer.data();
    }

#if defined(_WIN32)
    (void)_pclose(pipe);
#else
    (void)pclose(pipe);
#endif
    return output;
}

std::string trim(std::string s)
{
    const auto notSpace = [](unsigned char ch) { return std::isspace(ch) == 0; };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    return s;
}

nlohmann::json readJson(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        return nlohmann::json{};
    }
    nlohmann::json value;
    input >> value;
    return value;
}

nlohmann::json parseJsonText(const std::string& text)
{
    if (text.empty())
    {
        return nlohmann::json{};
    }
    return nlohmann::json::parse(text, nullptr, false);
}

nlohmann::json buildUserPayload(const std::filesystem::path& analysisDir, const SessionContext& context)
{
    auto result = context.liveAnalysisJson.empty() ? readJson(analysisDir / "result.json")
                                                   : parseJsonText(context.liveAnalysisJson);
    auto fixList = context.liveFixCandidatesJson.empty() ? readJson(analysisDir / "fix_list.json")
                                                         : parseJsonText(context.liveFixCandidatesJson);

  
    nlohmann::json payload;
    payload["genre"] = context.genre;
    payload["user_intent"] = context.userIntent;
    payload["constraints"] = context.constraints;
    payload["conversation_id"] = context.conversationId;
    payload["prompt"] = context.prompt;
    payload["analysis"] = result.is_object() ? result : nlohmann::json::object();
    payload["analysis_fix_candidates"] = fixList.is_array() ? fixList : nlohmann::json::array();
    payload["requirements"] = {
        {"live_buffer", true},
        {"reference.pool.validate", true},
        {"suggest_not_command", true},
        {"no_judgement", true},
        {"ground_every_issue_in_metrics", false},
        {"evolve_feedback_and_adjust_tone", true},
        {"slight_dry_wit", true},
        {"hallucinate_metrics_fake_feedback", false}
    };
    return payload;
}

std::string extractTextResponse(const nlohmann::json& payload)
{
    if (!payload.is_object())
    {
        return {};
    }
    if (payload.contains("output_text") && payload["output_text"].is_string())
    {
        return trim(payload["output_text"].get<std::string>());
    }
    if (payload.contains("output") && payload["output"].is_array())
    {
        for (const auto& item : payload["output"])
        {
            if (!item.is_object() || !item.contains("content") || !item["content"].is_array())
            {
                continue;
            }
            for (const auto& content : item["content"])
            {
                if (content.contains("text") && content["text"].is_string())
                {
                    const auto text = trim(content["text"].get<std::string>());
                    if (!text.empty())
                    {
                        return text;
                    }
                }
            }
        }
    }
    return {};
}

std::string firstSentenceOrLine(const std::string& input)
{
    const auto trimmed = trim(input);
    if (trimmed.empty())
    {
        return {};
    }

    const auto sentenceEnd = trimmed.find('.');
    if (sentenceEnd != std::string::npos && sentenceEnd + 1 >= 16)
    {
        return trim(trimmed.substr(0, sentenceEnd + 1));
    }

    const auto lineEnd = trimmed.find('\n');
    if (lineEnd != std::string::npos)
    {
        return trim(trimmed.substr(0, lineEnd));
    }
    return trimmed;
}

std::string joinNonEmpty(const std::vector<std::string>& values, const std::string& separator)
{
    std::string out;
    for (const auto& value : values)
    {
        const auto trimmed = trim(value);
        if (trimmed.empty())
        {
            continue;
        }
        if (!out.empty())
        {
            out += separator;
        }
        out += trimmed;
    }
    return out;
}

std::string buildDisplayDetails(const AdvisoryOutput& output)
{
    std::string details;

    const auto appendBlock = [&](const std::string& block)
    {
        const auto trimmed = trim(block);
        if (trimmed.empty())
        {
            return;
        }
        if (!details.empty())
        {
            details += "\n\n";
        }
        details += trimmed;
    };

    appendBlock(output.summary30s);

    for (const auto& issue : output.issues)
    {
        std::string block;
        const auto appendLine = [&block](const std::string& line)
        {
            const auto trimmed = trim(line);
            if (trimmed.empty())
            {
                return;
            }
            if (!block.empty())
            {
                block += "\n";
            }
            block += trimmed;
        };

        appendLine(issue.summary);
        appendLine(issue.whyItMatters);
        appendLine(joinNonEmpty(issue.tryFirst, "; "));
        appendLine(joinNonEmpty(issue.ignoreForNow, "; "));
        appendLine(joinNonEmpty(issue.evidencePointers, " | "));
        appendBlock(block);
    }

    std::vector<std::string> actionLabels;
    actionLabels.reserve(output.actionSuggestions.size());
    for (const auto& action : output.actionSuggestions)
    {
        const auto label = trim(action.label);
        if (!label.empty())
        {
            actionLabels.push_back(label);
        }
    }
    appendBlock(joinNonEmpty(actionLabels, "\n"));

    return details;
}

std::string buildSystemPrompt()
{
    return R"PROMPT(You are AIFR3D, a calm senior mix engineer that helps producers reach Industry Standard quality mixes by mathematically measuring the live buffer and comparing it to a reference pool that contains DSP metrics of professionally released tracks.
You speak concisely, honestly, and respectfully, praise improvement and ground users ego-based mixing confidence with data based objective, constructive but supporting feedback.
You never judge the producer and you never forbid creative choices.
You teach mixing fundamentals and help producers ensure every mix reaches professional targets.
Ground mix issues in the supplied live DSP metrics.
Be open to conversation but always bring the conversation back to music production related topics.
Do NOT hallucinate or output fake or exaggerated suggestions or advice. 
`analysis_fix_candidates` contains deterministic DSP evidence, not user-facing wording.
Use that evidence to write the fix list in your own plain English interpretation of the live-buffer and reference pool language based on user input and chat context.
Focus on issues based off of priority. 
Keep fields practical using comprehensional in depth explanions.
Keep each summary and why_it_matters string to a few brief sentences, ensuring user comprehension. 
Keep try_first and ignore_for_now based on most effective actions. Be descriptive. 
Return JSON only with this exact shape:
{
  "summary": "string",
  "issues": [
    {
      "issue_type": "string",
      "severity": "Low|Medium|High",
      "summary": "string",
      "why_it_matters": "string",
      "try_first": ["string"],
      "ignore_for_now": ["string"],
      "evidence": {"metrics": {}, "pointers": ["string"]}
    }
  ],
  "action_suggestions": [
    {
      "id": "string",
      "label": "string",
      "payload": {},
      "destructive": false,
      "requiresSave": false
    }
  ],
  "state_update": {
    "focus_tags": ["string"],
    "signal_clarity": 0.0,
    "confidence": 0.0
  }
})PROMPT";
}

std::optional<nlohmann::json> runViaOpenAi(const std::string& apiKey, const std::string& model,
                                           const std::string& endpoint,
                                           const nlohmann::json& userPayload)
{
    if (apiKey.empty() || !commandAvailable("curl"))
    {
        return std::nullopt;
    }

    const auto useChatCompletions = endpoint.find("/chat/completions") != std::string::npos;
    nlohmann::json body = useChatCompletions
                              ? nlohmann::json{{"model", model},
                                               {"messages",
                                                {{{"role", "system"}, {"content", buildSystemPrompt()}},
                                                 {{"role", "user"}, {"content", userPayload.dump()}}}},
                                               {"max_tokens", 900},
                                               {"response_format", {{"type", "json_object"}}}}
                              : nlohmann::json{{"model", model},
                                               {"input",
                                                {{{"role", "system"},
                                                  {"content", {{{"type", "input_text"}, {"text", buildSystemPrompt()}}}}},
                                                 {{"role", "user"},
                                                  {"content", {{{"type", "input_text"}, {"text", userPayload.dump()}}}}}}},
                                               {"max_output_tokens", 900},
                                               {"text", {{"format", {{"type", "json_object"}}},
                                                         {"verbosity", "medium"}}}};

    const std::string command =
        "curl -sS --connect-timeout 4 --max-time 120 -H " +
        shellEscapeSingleQuoted("Authorization: Bearer " + apiKey) +
        " -H 'Content-Type: application/json' -d " + shellEscapeSingleQuoted(body.dump()) +
        " " + shellEscapeSingleQuoted(endpoint);

    const auto raw = trim(runShellCommand(command));
    if (raw.empty())
    {
        return std::nullopt;
    }

    auto payload = nlohmann::json::parse(raw, nullptr, false);
    if (payload.is_discarded())
    {
        return std::nullopt;
    }

    const auto text = extractTextResponse(payload);
    if (text.empty())
    {
        return std::nullopt;
    }

    auto structured = nlohmann::json::parse(text, nullptr, false);
    if (structured.is_discarded() || !structured.is_object())
    {
        return nlohmann::json{
            {"summary", text},
            {"issues", nlohmann::json::array()},
            {"action_suggestions", nlohmann::json::array()},
            {"state_update", nlohmann::json::object()}
        };
    }
    return structured;
}

} // namespace

std::optional<AdvisoryOutput> OpenAiAdviser::advise(const std::filesystem::path& analysisDir,
                                                    const SessionContext& context)
{
    const auto structured = runViaOpenAi(m_apiKey, m_model, m_endpoint,
                                         buildUserPayload(analysisDir, context));
    if (!structured.has_value())
    {
        return std::nullopt;
    }

    AdvisoryOutput out;
    out.summary30s = trim(structured->value("summary", ""));

    if (structured->contains("issues") && (*structured)["issues"].is_array())
    {
        for (const auto& issue : (*structured)["issues"])
        {
            if (!issue.is_object())
            {
                continue;
            }
            AdvisoryIssue parsed;
            parsed.issueType = trim(issue.value("issue_type", ""));
            parsed.severity = trim(issue.value("severity", ""));
            parsed.summary = trim(issue.value("summary", ""));
            parsed.whyItMatters = trim(issue.value("why_it_matters", ""));
            if (issue.contains("try_first") && issue["try_first"].is_array())
            {
                for (const auto& item : issue["try_first"])
                {
                    if (item.is_string())
                    {
                        parsed.tryFirst.push_back(trim(item.get<std::string>()));
                    }
                }
            }
            if (issue.contains("ignore_for_now") && issue["ignore_for_now"].is_array())
            {
                for (const auto& item : issue["ignore_for_now"])
                {
                    if (item.is_string())
                    {
                        parsed.ignoreForNow.push_back(trim(item.get<std::string>()));
                    }
                }
            }
            if (issue.contains("evidence") && issue["evidence"].is_object() &&
                issue["evidence"].contains("pointers") && issue["evidence"]["pointers"].is_array())
            {
                for (const auto& item : issue["evidence"]["pointers"])
                {
                    if (item.is_string())
                    {
                        parsed.evidencePointers.push_back(trim(item.get<std::string>()));
                    }
                }
            }
            if (!parsed.summary.empty())
            {
                out.top3Fixes.push_back(parsed.summary);
                out.issues.push_back(std::move(parsed));
            }
            if (out.top3Fixes.size() >= 3)
            {
                break;
            }
        }
    }

    if (structured->contains("action_suggestions") && (*structured)["action_suggestions"].is_array())
    {
        for (const auto& suggestion : (*structured)["action_suggestions"])
        {
            if (!suggestion.is_object())
            {
                continue;
            }
            AdvisoryActionSuggestion parsed;
            parsed.id = trim(suggestion.value("id", ""));
            parsed.label = trim(suggestion.value("label", ""));
            parsed.destructive = suggestion.value("destructive", false);
            parsed.requiresSave = suggestion.value("requiresSave", false);
            if (!parsed.label.empty())
            {
                out.actionSuggestions.push_back(std::move(parsed));
            }
        }
    }

    const auto state = structured->value("state_update", nlohmann::json::object());
    out.signalClarity = state.value("signal_clarity", 0.0);
    out.confidence = state.value("confidence", out.signalClarity);

    if (!out.issues.empty() && !out.issues.front().tryFirst.empty())
    {
        out.safePath = out.issues.front().tryFirst.front();
    }

    if (out.issues.size() > 1 && !out.issues[1].tryFirst.empty())
    {
        out.boldPath = out.issues[1].tryFirst.front();
    }

    if (out.top3Fixes.empty() && !out.summary30s.empty())
    {
        out.top3Fixes.push_back(firstSentenceOrLine(out.summary30s));
    }

    out.details = buildDisplayDetails(out);
    if (out.summary30s.empty() && out.top3Fixes.empty() && out.details.empty())
    {
        return std::nullopt;
    }

    return out;
}

} // namespace dawai::advisory_layer
