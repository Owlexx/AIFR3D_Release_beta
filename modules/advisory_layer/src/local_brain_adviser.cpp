#include "dawai/advisory_layer/local_brain_adviser.hpp"

#include "dawai/advisory_layer/api_key_store.hpp"
#include "dawai/advisory_layer/openai_adviser.hpp"
#include "dawai/aifr3d_core/system_contract.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace dawai::advisory_layer
{

namespace
{
constexpr auto kOpenAiModel = "gpt-5.2";

std::string trim(std::string value)
{
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch)
                                            { return std::isspace(ch) == 0; }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [](unsigned char ch) { return std::isspace(ch) == 0; })
                    .base(),
                value.end());
    return value;
}

std::string openAiApiKey()
{
    return ApiKeyStore::loadOpenAiKey();
}

} // namespace

LocalBrainAdviser::LocalBrainAdviser(std::vector<std::string> modelNames, std::string endpoint)
{
    const auto key = openAiApiKey();
    if (key.empty())
    {
        return;
    }

    const auto requestedName =
        modelNames.empty() ? std::string(kOpenAiModel) : trim(modelNames.front());
    const auto model = requestedName.empty() ? std::string(kOpenAiModel) : requestedName;
    m_chain.push_back({model, std::make_unique<OpenAiAdviser>(model, key, endpoint)});
}

std::optional<AdvisoryOutput> LocalBrainAdviser::advise(const std::filesystem::path& analysisDir,
                                                        const SessionContext& context)
{
    SessionContext enriched = context;
    const auto memoryText = m_memory.loadConversationContext(context.conversationId);
    const auto attachmentText = m_memory.loadAttachmentContext(context.attachments);

    if (!memoryText.empty())
    {
        if (!enriched.constraints.empty())
        {
            enriched.constraints += "\n";
        }
        enriched.constraints += "Conversation memory:\n" + memoryText;
    }
    if (!attachmentText.empty())
    {
        if (!enriched.constraints.empty())
        {
            enriched.constraints += "\n";
        }
        enriched.constraints += "Attachment context:\n" + attachmentText;
    }

    for (auto& entry : m_chain)
    {
        if (!entry.adviser)
        {
            continue;
        }

        auto output = entry.adviser->advise(analysisDir, enriched);
        if (output)
        {
            output->details += "\n\nBrain source: " + entry.label;
            m_memory.appendTurn(context.conversationId,
                                context.prompt.empty() ? context.userIntent : context.prompt,
                                output->details, context.attachments);
            return output;
        }
    }

    return std::nullopt;
}

std::unique_ptr<IAdviser> LocalBrainAdviser::createFromEnvironment()
{
    return std::make_unique<LocalBrainAdviser>(std::vector<std::string>{kOpenAiModel},
                                               "https://api.openai.com/v1/responses");
}

} // namespace dawai::advisory_layer
