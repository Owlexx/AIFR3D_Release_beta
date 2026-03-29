#include "dawai/advisory_layer/api_key_store.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <array>
#include <string_view>
#include <vector>

namespace dawai::advisory_layer
{

namespace
{
std::filesystem::path indexedKeyPath()
{
    if (const char* envPath = std::getenv("DAWAI_OPENAI_API_KEY_FILE");
        envPath != nullptr && *envPath != '\0')
    {
        return std::filesystem::path(envPath);
    }
    return std::filesystem::path("/home/north3rnlight3r/Documents/API_KEY_INDEX/OpenAI API Key.txt");
}

std::filesystem::path keyStorePath()
{
    if (const char* envPath = std::getenv("DAWAI_OPENAI_KEY_STORE");
        envPath != nullptr && *envPath != '\0')
    {
        return std::filesystem::path(envPath);
    }

#if defined(_WIN32)
    const char* home = std::getenv("USERPROFILE");
#else
    const char* home = std::getenv("HOME");
#endif
    if (home == nullptr || *home == '\0')
    {
        return std::filesystem::path("analysis") / "openai_api_key.enc";
    }
    return std::filesystem::path(home) / ".config" / "dawai" / "openai_api_key.enc";
}

std::uint8_t keyByte()
{
    const auto path = keyStorePath().string();
    std::size_t hash = std::hash<std::string>{}(path);
#if defined(_WIN32)
    const char* user = std::getenv("USERNAME");
#else
    const char* user = std::getenv("USER");
#endif
    if (user != nullptr)
    {
        hash ^= std::hash<std::string>{}(user);
    }
    return static_cast<std::uint8_t>((hash & 0xFFu) ^ 0xA5u);
}

std::string xorTransform(const std::string& in)
{
    std::string out = in;
    const auto k = keyByte();
    for (char& ch : out)
    {
        ch = static_cast<char>(static_cast<std::uint8_t>(ch) ^ k);
    }
    return out;
}

std::string toHex(std::string_view input)
{
    static constexpr const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char byte : input)
    {
        out.push_back(kHex[(byte >> 4) & 0x0F]);
        out.push_back(kHex[byte & 0x0F]);
    }
    return out;
}

int hexValue(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return 10 + (ch - 'A');
    }
    return -1;
}

std::string fromHex(std::string_view input)
{
    if ((input.size() % 2) != 0)
    {
        return {};
    }

    std::string out;
    out.reserve(input.size() / 2);
    for (std::size_t i = 0; i < input.size(); i += 2)
    {
        const int hi = hexValue(input[i]);
        const int lo = hexValue(input[i + 1]);
        if (hi < 0 || lo < 0)
        {
            return {};
        }
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}

std::string parseOpenAiKey(const std::string& raw)
{
    const auto directPos = raw.find("sk-");
    if (directPos != std::string::npos)
    {
        std::string token;
        for (std::size_t i = directPos; i < raw.size(); ++i)
        {
            const char ch = raw[i];
            if (std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '-' || ch == '_')
            {
                token.push_back(ch);
            }
            else if (!token.empty())
            {
                break;
            }
        }
        if (token.size() > 20)
        {
            return token;
        }
    }
    return {};
}

std::string trimWhitespace(std::string value)
{
    const auto isNotSpace = [](unsigned char ch) { return std::isspace(ch) == 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), isNotSpace).base(), value.end());
    return value;
}

std::string readIndexedKey()
{
    const auto path = indexedKeyPath();
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || std::filesystem::is_directory(path, ec))
    {
        return {};
    }

    std::ifstream input(path);
    if (!input)
    {
        return {};
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    return parseOpenAiKey(buffer.str());
}

} // namespace

std::string ApiKeyStore::normalizeOpenAiKey(const std::string& apiKey)
{
    const auto parsed = parseOpenAiKey(apiKey);
    if (!parsed.empty())
    {
        return parsed;
    }
    return trimWhitespace(apiKey);
}

bool ApiKeyStore::saveOpenAiKey(const std::string& apiKey)
{
    const auto normalized = normalizeOpenAiKey(apiKey);
    if (normalized.empty())
    {
        return false;
    }

    const auto indexedPath = indexedKeyPath();
    std::filesystem::create_directories(indexedPath.parent_path());
    std::ofstream indexedOut(indexedPath, std::ios::trunc);
    if (!indexedOut)
    {
        return false;
    }
    indexedOut << normalized << '\n';
    indexedOut.flush();
    if (!indexedOut)
    {
        return false;
    }

    const auto path = keyStorePath();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::trunc);
    if (!out)
    {
        return true;
    }
    const auto encrypted = xorTransform(normalized);
    out << toHex(encrypted);
    out.flush();
    return true;
}

std::string ApiKeyStore::loadOpenAiKey()
{
    return readIndexedKey();
}

bool ApiKeyStore::clearOpenAiKey()
{
    std::error_code ec;
    const bool removedPrimary = std::filesystem::remove(indexedKeyPath(), ec);
    ec.clear();
    const bool removedStore = std::filesystem::remove(keyStorePath(), ec);
    return removedPrimary || removedStore;
}

} // namespace dawai::advisory_layer
