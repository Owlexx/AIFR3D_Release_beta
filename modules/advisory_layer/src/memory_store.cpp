#include "dawai/advisory_layer/memory_store.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace dawai::advisory_layer
{

namespace
{
std::string readTextFile(const std::filesystem::path& path, std::size_t maxBytes)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
    {
        return {};
    }

    std::string buffer;
    buffer.resize(maxBytes);
    input.read(buffer.data(), static_cast<std::streamsize>(maxBytes));
    buffer.resize(static_cast<std::size_t>(input.gcount()));
    return buffer;
}

std::string trimToBytes(std::string value, std::size_t maxBytes)
{
    if (value.size() > maxBytes)
    {
        value.resize(maxBytes);
    }
    return value;
}

std::size_t readEnvSize(const char* key, std::size_t fallback, std::size_t minValue,
                        std::size_t maxValue)
{
    const char* raw = std::getenv(key);
    if (raw == nullptr || *raw == '\0')
    {
        return fallback;
    }

    try
    {
        const auto parsed = static_cast<std::size_t>(std::stoull(raw));
        return std::clamp(parsed, minValue, maxValue);
    }
    catch (...)
    {
        return fallback;
    }
}

} // namespace

std::filesystem::path MemoryStore::conversationPath(const std::string& conversationId) const
{
    return m_root / (sanitizeConversationId(conversationId) + ".json");
}

std::string MemoryStore::sanitizeConversationId(const std::string& raw)
{
    std::string out;
    out.reserve(raw.size());
    for (const char c : raw)
    {
        if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '_')
        {
            out.push_back(c);
        }
        else
        {
            out.push_back('_');
        }
    }
    if (out.empty())
    {
        return "default";
    }
    return out;
}

std::string MemoryStore::loadConversationContext(const std::string& conversationId,
                                                 std::size_t maxTurns,
                                                 std::size_t maxBytes) const
{
    std::ifstream input(conversationPath(conversationId));
    if (!input)
    {
        return {};
    }

    nlohmann::json turns;
    input >> turns;
    if (!turns.is_array() || turns.empty())
    {
        return {};
    }

    const std::size_t start = (turns.size() > maxTurns) ? (turns.size() - maxTurns) : 0;
    std::ostringstream context;
    std::size_t used = 0;

    for (std::size_t i = start; i < turns.size(); ++i)
    {
        const auto& turn = turns[i];
        std::ostringstream chunk;
        chunk << "[User] " << turn.value("prompt", "") << "\n";
        chunk << "[Assistant] " << turn.value("response", "") << "\n";
        const auto text = chunk.str();
        if (used + text.size() > maxBytes)
        {
            break;
        }
        context << text;
        used += text.size();
    }

    return context.str();
}

std::string
MemoryStore::loadAttachmentContext(const std::vector<std::filesystem::path>& attachments,
                                   std::size_t maxBytesPerFile) const
{
    if (attachments.empty())
    {
        return {};
    }

    std::ostringstream merged;
    std::size_t loaded = 0;
    for (const auto& path : attachments)
    {
        if (!std::filesystem::is_regular_file(path))
        {
            continue;
        }
        const auto text = readTextFile(path, maxBytesPerFile);
        if (text.empty())
        {
            continue;
        }
        merged << "File: " << path.string() << "\n";
        merged << trimToBytes(text, maxBytesPerFile) << "\n\n";
        ++loaded;
        if (loaded >= 3)
        {
            break;
        }
    }
    return merged.str();
}

void MemoryStore::appendTurn(const std::string& conversationId, const std::string& prompt,
                             const std::string& response,
                             const std::vector<std::filesystem::path>& attachments) const
{
    std::filesystem::create_directories(m_root);
    const auto path = conversationPath(conversationId);

    nlohmann::json turns = nlohmann::json::array();
    {
        std::ifstream input(path);
        if (input)
        {
            input >> turns;
            if (!turns.is_array())
            {
                turns = nlohmann::json::array();
            }
        }
    }

    nlohmann::json turn;
    turn["timestamp_utc"] = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
    turn["prompt"] = prompt;
    turn["response"] = response;
    turn["attachments"] = nlohmann::json::array();
    for (const auto& attachment : attachments)
    {
        turn["attachments"].push_back(attachment.string());
    }

    turns.push_back(std::move(turn));

    if (turns.size() > maxStoredTurns())
    {
        turns.erase(turns.begin(),
                    turns.begin() +
                        static_cast<std::ptrdiff_t>(turns.size() - maxStoredTurns()));
    }

    const std::size_t maxBytes =
        readEnvSize("DAWAI_MEMORY_MAX_BYTES", 512 * 1024, 64 * 1024, 10 * 1024 * 1024);
    std::string serialized = turns.dump(2);
    while (serialized.size() > maxBytes && turns.size() > 1)
    {
        turns.erase(turns.begin());
        serialized = turns.dump(2);
    }

    std::ofstream output(path, std::ios::trunc);
    output << serialized;
}

std::size_t MemoryStore::maxStoredTurns()
{
    return readEnvSize("DAWAI_MEMORY_MAX_TURNS", 7, 1, 50);
}

} // namespace dawai::advisory_layer
