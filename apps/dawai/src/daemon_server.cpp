#include "daemon_server.hpp"

#include "dawai/advisory_layer/api_key_store.hpp"
#include "dawai/advisory_layer/local_brain_adviser.hpp"
#include "dawai/aifr3d_core/system_contract.hpp"

#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace dawai
{

namespace
{

constexpr int kSocketWaitMs = 10000;
constexpr std::size_t kMaxHeaderBytes = 64 * 1024;
constexpr std::size_t kMaxBodyBytes = 32 * 1024 * 1024;
constexpr std::size_t kMaxWebSocketPayload = 8 * 1024 * 1024;
constexpr auto kMissingKeyFeedback =
    "Feedback not available Please install OpenAPI Key to use AIFRED";
constexpr auto kOfflineFeedback =
    "AIFRED IS NOT ONLINE PLEASE CONNECT TO INTERNET TO USE AIFRED";

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string trim(std::string value)
{
    const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](char c)
                                            { return notSpace(static_cast<unsigned char>(c)); }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
                             [&](char c) { return notSpace(static_cast<unsigned char>(c)); })
                    .base(),
                value.end());
    return value;
}

std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string safeToken(std::string value, const std::string& fallback)
{
    std::string out;
    out.reserve(value.size());

    for (char ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '-')
        {
            out.push_back(ch);
        }
    }

    if (out.empty())
    {
        return fallback;
    }
    return out;
}

std::string advisoryUnavailableMessage()
{
    return dawai::advisory_layer::ApiKeyStore::loadOpenAiKey().empty() ? kMissingKeyFeedback
                                                                       : kOfflineFeedback;
}

std::map<std::string, std::string> parseQuery(const std::string& query)
{
    std::map<std::string, std::string> out;
    std::stringstream ss(query);
    std::string entry;
    while (std::getline(ss, entry, '&'))
    {
        if (entry.empty())
        {
            continue;
        }

        const auto eqPos = entry.find('=');
        const std::string key = eqPos == std::string::npos ? entry : entry.substr(0, eqPos);
        const std::string value = eqPos == std::string::npos ? "" : entry.substr(eqPos + 1);
        out[toLower(trim(key))] = trim(value);
    }

    return out;
}

bool readExact(juce::StreamingSocket& socket, void* outData, std::size_t bytes)
{
    std::size_t readBytes = 0;
    auto* out = static_cast<char*>(outData);

    while (readBytes < bytes)
    {
        if (socket.waitUntilReady(true, kSocketWaitMs) <= 0)
        {
            return false;
        }

        const auto remaining = static_cast<int>(bytes - readBytes);
        const int got = socket.read(out + readBytes, remaining, false);
        if (got <= 0)
        {
            return false;
        }
        readBytes += static_cast<std::size_t>(got);
    }

    return true;
}

bool writeAll(juce::StreamingSocket& socket, const std::string& data)
{
    std::size_t sent = 0;
    while (sent < data.size())
    {
        if (socket.waitUntilReady(false, kSocketWaitMs) <= 0)
        {
            return false;
        }

        const auto remaining = static_cast<int>(data.size() - sent);
        const int wrote = socket.write(data.data() + sent, remaining);
        if (wrote <= 0)
        {
            return false;
        }
        sent += static_cast<std::size_t>(wrote);
    }
    return true;
}

bool readHttpRequest(juce::StreamingSocket& socket, HttpRequest& request)
{
    std::string buffer;
    buffer.reserve(4096);

    while (buffer.find("\r\n\r\n") == std::string::npos)
    {
        if (socket.waitUntilReady(true, kSocketWaitMs) <= 0)
        {
            return false;
        }

        char chunk[4096];
        const int got = socket.read(chunk, static_cast<int>(sizeof(chunk)), false);
        if (got <= 0)
        {
            return false;
        }
        buffer.append(chunk, static_cast<std::size_t>(got));

        if (buffer.size() > kMaxHeaderBytes)
        {
            return false;
        }
    }

    const auto headerEnd = buffer.find("\r\n\r\n");
    const std::string headerBlock = buffer.substr(0, headerEnd);
    std::string body = buffer.substr(headerEnd + 4);

    std::istringstream stream(headerBlock);
    std::string firstLine;
    if (!std::getline(stream, firstLine))
    {
        return false;
    }
    if (!firstLine.empty() && firstLine.back() == '\r')
    {
        firstLine.pop_back();
    }

    std::istringstream firstLineParts(firstLine);
    std::string target;
    std::string version;
    if (!(firstLineParts >> request.method >> target >> version))
    {
        return false;
    }
    request.method = toLower(trim(request.method));

    const auto qPos = target.find('?');
    if (qPos == std::string::npos)
    {
        request.path = target;
    }
    else
    {
        request.path = target.substr(0, qPos);
        request.query = target.substr(qPos + 1);
    }

    std::string line;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.empty())
        {
            continue;
        }

        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos)
        {
            continue;
        }

        const std::string key = toLower(trim(line.substr(0, colonPos)));
        const std::string value = trim(line.substr(colonPos + 1));
        request.headers[key] = value;
    }

    std::size_t contentLength = 0;
    if (const auto it = request.headers.find("content-length"); it != request.headers.end())
    {
        try
        {
            contentLength = static_cast<std::size_t>(std::max(0, std::stoi(it->second)));
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    if (contentLength > kMaxBodyBytes)
    {
        return false;
    }

    while (body.size() < contentLength)
    {
        if (socket.waitUntilReady(true, kSocketWaitMs) <= 0)
        {
            return false;
        }
        char chunk[4096];
        const int got = socket.read(chunk, static_cast<int>(sizeof(chunk)), false);
        if (got <= 0)
        {
            return false;
        }
        body.append(chunk, static_cast<std::size_t>(got));
    }

    if (body.size() > contentLength)
    {
        body.resize(contentLength);
    }
    request.body = std::move(body);
    return true;
}

void sendHttpResponse(juce::StreamingSocket& socket, int statusCode, const std::string& statusText,
                      const std::string& body, const std::string& contentType,
                      const std::vector<std::string>& extraHeaders = {})
{
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Access-Control-Allow-Headers: Authorization, Content-Type\r\n";
    response << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    for (const auto& header : extraHeaders)
    {
        response << header << "\r\n";
    }
    response << "Connection: close\r\n\r\n";
    response << body;
    (void)writeAll(socket, response.str());
}

std::string readEnv(const char* key)
{
    const char* value = std::getenv(key);
    return value != nullptr ? std::string(value) : std::string();
}

std::string requiredAuthToken()
{
    const std::array<const char*, 4> envKeys = {
        "DAWAI_DAEMON_TOKEN",
        "DAWAI_API_TOKEN",
        "DAWAI_CLOUD_API_TOKEN",
        "CLOUDFLARE_API_TOKEN",
    };

    for (const auto* key : envKeys)
    {
        const auto value = trim(readEnv(key));
        if (!value.empty())
        {
            return value;
        }
    }
    return {};
}

bool authorized(const HttpRequest& request, const std::string& requiredToken)
{
    if (requiredToken.empty())
    {
        return true;
    }

    if (auto it = request.headers.find("authorization"); it != request.headers.end())
    {
        constexpr std::string_view bearerPrefix = "Bearer ";
        if (it->second.rfind(bearerPrefix.data(), 0) == 0)
        {
            const auto provided = it->second.substr(bearerPrefix.size());
            if (provided == requiredToken)
            {
                return true;
            }
        }
    }

    const auto query = parseQuery(request.query);
    if (auto it = query.find("token"); it != query.end())
    {
        if (it->second == requiredToken)
        {
            return true;
        }
    }

    return false;
}

std::uint32_t rotateLeft(std::uint32_t value, std::uint32_t bits)
{
    return (value << bits) | (value >> (32 - bits));
}

std::array<std::uint8_t, 20> sha1Digest(const std::string& input)
{
    std::vector<std::uint8_t> message(input.begin(), input.end());
    const std::uint64_t bitLength = static_cast<std::uint64_t>(message.size()) * 8ULL;

    message.push_back(0x80u);
    while ((message.size() % 64u) != 56u)
    {
        message.push_back(0u);
    }

    for (int i = 7; i >= 0; --i)
    {
        message.push_back(static_cast<std::uint8_t>((bitLength >> (i * 8)) & 0xFFu));
    }

    std::uint32_t h0 = 0x67452301u;
    std::uint32_t h1 = 0xEFCDAB89u;
    std::uint32_t h2 = 0x98BADCFEu;
    std::uint32_t h3 = 0x10325476u;
    std::uint32_t h4 = 0xC3D2E1F0u;

    for (std::size_t offset = 0; offset < message.size(); offset += 64)
    {
        std::array<std::uint32_t, 80> w{};
        for (std::size_t i = 0; i < 16; ++i)
        {
            const std::size_t idx = offset + i * 4;
            w[i] = (static_cast<std::uint32_t>(message[idx]) << 24) |
                   (static_cast<std::uint32_t>(message[idx + 1]) << 16) |
                   (static_cast<std::uint32_t>(message[idx + 2]) << 8) |
                   static_cast<std::uint32_t>(message[idx + 3]);
        }

        for (std::size_t i = 16; i < 80; ++i)
        {
            w[i] = rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        std::uint32_t a = h0;
        std::uint32_t b = h1;
        std::uint32_t c = h2;
        std::uint32_t d = h3;
        std::uint32_t e = h4;

        for (std::size_t i = 0; i < 80; ++i)
        {
            std::uint32_t f = 0;
            std::uint32_t k = 0;
            if (i < 20)
            {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999u;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1u;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDCu;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6u;
            }

            const std::uint32_t temp = rotateLeft(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotateLeft(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::array<std::uint8_t, 20> digest{};
    const std::array<std::uint32_t, 5> hashes{h0, h1, h2, h3, h4};
    for (std::size_t i = 0; i < hashes.size(); ++i)
    {
        digest[i * 4] = static_cast<std::uint8_t>((hashes[i] >> 24) & 0xFFu);
        digest[i * 4 + 1] = static_cast<std::uint8_t>((hashes[i] >> 16) & 0xFFu);
        digest[i * 4 + 2] = static_cast<std::uint8_t>((hashes[i] >> 8) & 0xFFu);
        digest[i * 4 + 3] = static_cast<std::uint8_t>(hashes[i] & 0xFFu);
    }

    return digest;
}

std::string websocketAcceptKey(const std::string& secWebSocketKey)
{
    constexpr std::string_view kGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const auto digest = sha1Digest(secWebSocketKey + std::string(kGuid));
    return juce::Base64::toBase64(digest.data(), digest.size()).toStdString();
}

struct WsFrame
{
    bool fin = false;
    std::uint8_t opcode = 0;
    std::string payload;
};

bool readWsFrame(juce::StreamingSocket& socket, WsFrame& frame)
{
    std::uint8_t header[2] = {};
    if (!readExact(socket, header, sizeof(header)))
    {
        return false;
    }

    frame.fin = (header[0] & 0x80u) != 0;
    frame.opcode = static_cast<std::uint8_t>(header[0] & 0x0Fu);

    const bool masked = (header[1] & 0x80u) != 0;
    std::uint64_t payloadLength = static_cast<std::uint64_t>(header[1] & 0x7Fu);

    if (payloadLength == 126u)
    {
        std::uint8_t ext[2] = {};
        if (!readExact(socket, ext, sizeof(ext)))
        {
            return false;
        }
        payloadLength = (static_cast<std::uint64_t>(ext[0]) << 8) | ext[1];
    }
    else if (payloadLength == 127u)
    {
        std::uint8_t ext[8] = {};
        if (!readExact(socket, ext, sizeof(ext)))
        {
            return false;
        }

        payloadLength = 0;
        for (std::uint8_t part : ext)
        {
            payloadLength = (payloadLength << 8) | static_cast<std::uint64_t>(part);
        }
    }

    if (payloadLength > kMaxWebSocketPayload)
    {
        return false;
    }

    std::uint8_t mask[4] = {};
    if (masked && !readExact(socket, mask, sizeof(mask)))
    {
        return false;
    }

    std::string payload;
    payload.resize(static_cast<std::size_t>(payloadLength));
    if (payloadLength > 0 && !readExact(socket, payload.data(), payload.size()))
    {
        return false;
    }

    if (masked)
    {
        for (std::size_t i = 0; i < payload.size(); ++i)
        {
            payload[i] = static_cast<char>(payload[i] ^ mask[i % 4]);
        }
    }

    frame.payload = std::move(payload);
    return true;
}

bool sendWsFrame(juce::StreamingSocket& socket, std::uint8_t opcode, const std::string& payload)
{
    std::string frame;
    frame.reserve(payload.size() + 16);
    frame.push_back(static_cast<char>(0x80u | (opcode & 0x0Fu)));

    const auto payloadSize = static_cast<std::uint64_t>(payload.size());
    if (payloadSize < 126u)
    {
        frame.push_back(static_cast<char>(payloadSize));
    }
    else if (payloadSize <= 0xFFFFu)
    {
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((payloadSize >> 8) & 0xFFu));
        frame.push_back(static_cast<char>(payloadSize & 0xFFu));
    }
    else
    {
        frame.push_back(static_cast<char>(127));
        for (int shift = 56; shift >= 0; shift -= 8)
        {
            frame.push_back(static_cast<char>((payloadSize >> shift) & 0xFFu));
        }
    }
    frame.append(payload);
    return writeAll(socket, frame);
}

std::string nowIsoUtc()
{
    return juce::Time::getCurrentTime().toISO8601(true).toStdString();
}

std::string shellQuote(const std::string& value)
{
    std::string out = "'";
    for (char ch : value)
    {
        if (ch == '\'')
        {
            out += "'\\''";
        }
        else
        {
            out.push_back(ch);
        }
    }
    out.push_back('\'');
    return out;
}

nlohmann::json runShellCommand(const std::string& commandLine)
{
    juce::ChildProcess process;

    std::string launch;
#if JUCE_WINDOWS
    launch = "cmd /C \"" + commandLine + "\"";
#else
    launch = "/bin/bash -lc " + shellQuote(commandLine);
#endif

    if (!process.start(launch))
    {
        return nlohmann::json{{"ok", false},
                              {"exit_code", -1},
                              {"stdout", ""},
                              {"stderr", "Failed to start command process."}};
    }

    const bool finished = process.waitForProcessToFinish(120000);
    const auto output = process.readAllProcessOutput().toStdString();
    const auto exitCode = process.getExitCode();

    if (!finished)
    {
        process.kill();
        return nlohmann::json{{"ok", false},
                              {"exit_code", -1},
                              {"stdout", output},
                              {"stderr", "Command timed out after 120s."}};
    }

    return nlohmann::json{{"ok", exitCode == 0},
                          {"exit_code", exitCode},
                          {"stdout", output},
                          {"stderr", exitCode == 0 ? "" : output}};
}

nlohmann::json doctorReport(int port)
{
    return nlohmann::json{{"ok", true},
                          {"service", "dawai-daemon"},
                          {"port", port},
                          {"system_id", dawai::aifr3d_core::contract::kSystemId},
                          {"metric_math_pack", dawai::aifr3d_core::contract::kMetricMathPack},
                          {"brain_config_path", dawai::aifr3d_core::contract::kBrainConfigPath},
                          {"report_schema_path", dawai::aifr3d_core::contract::kReportSchemaPath},
                          {"timestamp_utc", nowIsoUtc()}};
}

nlohmann::json executeCommandLine(const std::string& commandLine, int port)
{
    const auto trimmed = trim(commandLine);
    if (trimmed.empty())
    {
        return nlohmann::json{
            {"ok", false}, {"exit_code", 2}, {"stdout", ""}, {"stderr", "Command is empty."}};
    }

    if (trimmed == "dawai doctor")
    {
        return nlohmann::json{
            {"ok", true}, {"exit_code", 0}, {"stdout", doctorReport(port).dump(2)}, {"stderr", ""}};
    }

    if (trimmed == "dawai daemon status")
    {
        return nlohmann::json{
            {"ok", true}, {"exit_code", 0}, {"stdout", doctorReport(port).dump(2)}, {"stderr", ""}};
    }

    return runShellCommand(trimmed);
}

std::vector<std::filesystem::path> materializeAttachments(const nlohmann::json& attachments,
                                                          const std::string& conversationId)
{
    std::vector<std::filesystem::path> paths;
    if (!attachments.is_array())
    {
        return paths;
    }

    const auto safeConversation = safeToken(conversationId, "default");
    const auto base = std::filesystem::path("analysis") / "attachments" / safeConversation;
    std::filesystem::create_directories(base);

    std::size_t count = 0;
    for (const auto& entry : attachments)
    {
        if (!entry.is_object())
        {
            continue;
        }
        const auto name = safeToken(entry.value("name", "attachment"), "attachment");
        const auto content = entry.value("content", "");
        if (content.empty())
        {
            continue;
        }

        const auto filePath =
            base / (std::to_string(juce::Time::currentTimeMillis()) + "_" + name + ".txt");
        std::ofstream output(filePath);
        output << content;
        paths.push_back(filePath);
        ++count;
        if (count >= 8)
        {
            break;
        }
    }

    return paths;
}

nlohmann::json unavailableChat(const std::string& genre, const std::string& conversationId)
{
    return nlohmann::json{{"ok", false},
                          {"error", advisoryUnavailableMessage()},
                          {"genre", genre},
                          {"conversation_id", conversationId}};
}

nlohmann::json makeUploadResponse(const std::filesystem::path& path, const std::string& target)
{
    return nlohmann::json{{"ok", true},
                          {"target", target},
                          {"stored_path", path.string()},
                          {"timestamp_utc", nowIsoUtc()}};
}

} // namespace

DaemonServer::DaemonServer(int port) : m_port(port) {}

DaemonServer::~DaemonServer()
{
    stop();
}

bool DaemonServer::start()
{
    if (m_running.exchange(true))
    {
        return true;
    }

    ensureAutoDependencies();

    m_listener = std::make_unique<juce::StreamingSocket>();
    if (!m_listener->createListener(m_port, "0.0.0.0"))
    {
        m_running.store(false);
        m_listener.reset();
        juce::Logger::writeToLog("Daemon failed to bind on port " + std::to_string(m_port));
        return false;
    }

    m_thread = std::thread([this]() { runLoop(); });
    juce::Logger::writeToLog("Daemon started on port " + std::to_string(m_port));
    return true;
}

int DaemonServer::runBlocking()
{
    if (!m_running.exchange(true))
    {
        ensureAutoDependencies();
        m_listener = std::make_unique<juce::StreamingSocket>();
        if (!m_listener->createListener(m_port, "0.0.0.0"))
        {
            m_running.store(false);
            m_listener.reset();
            juce::Logger::writeToLog("Daemon failed to bind on port " + std::to_string(m_port));
            return 1;
        }
    }

    juce::Logger::writeToLog("Daemon running in blocking mode on port " + std::to_string(m_port));
    runLoop();
    return 0;
}

void DaemonServer::stop()
{
    if (!m_running.exchange(false))
    {
        return;
    }

    if (m_listener)
    {
        m_listener->close();
    }

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    m_listener.reset();
    juce::Logger::writeToLog("Daemon stopped.");
}

void DaemonServer::runLoop()
{
    while (m_running.load())
    {
        if (m_listener == nullptr)
        {
            break;
        }

        auto* rawClient = m_listener->waitForNextConnection();
        if (rawClient == nullptr)
        {
            if (!m_running.load())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        auto client = std::unique_ptr<juce::StreamingSocket>(rawClient);
        std::thread([this, c = std::move(client)]() mutable { handleClient(std::move(c)); })
            .detach();
    }
}

void DaemonServer::handleClient(std::unique_ptr<juce::StreamingSocket> client) const
{
    if (!client)
    {
        return;
    }

    HttpRequest request;
    if (!readHttpRequest(*client, request))
    {
        sendHttpResponse(*client, 400, "Bad Request", R"({"error":"invalid request"})",
                         "application/json");
        client->close();
        return;
    }

    if (request.method == "options")
    {
        sendHttpResponse(*client, 204, "No Content", "", "text/plain");
        client->close();
        return;
    }

    if (!authorized(request, requiredAuthToken()))
    {
        sendHttpResponse(*client, 401, "Unauthorized", R"({"error":"unauthorized"})",
                         "application/json");
        client->close();
        return;
    }

    const bool wantsWebSocket = request.path == "/ws/chat" && request.headers.contains("upgrade") &&
                                toLower(request.headers["upgrade"]) == "websocket";

    if (wantsWebSocket)
    {
        const auto keyIt = request.headers.find("sec-websocket-key");
        if (keyIt == request.headers.end() || keyIt->second.empty())
        {
            sendHttpResponse(*client, 400, "Bad Request",
                             R"({"error":"missing sec-websocket-key"})", "application/json");
            client->close();
            return;
        }

        const auto accept = websocketAcceptKey(keyIt->second);
        std::ostringstream handshake;
        handshake << "HTTP/1.1 101 Switching Protocols\r\n";
        handshake << "Upgrade: websocket\r\n";
        handshake << "Connection: Upgrade\r\n";
        handshake << "Sec-WebSocket-Accept: " << accept << "\r\n\r\n";
        if (!writeAll(*client, handshake.str()))
        {
            client->close();
            return;
        }

        const auto sendJson = [&](const nlohmann::json& payload)
        { return sendWsFrame(*client, 0x1u, payload.dump()); };

        sendJson(nlohmann::json{{"type", "chat.ready"},
                                {"service", "dawai-daemon"},
                                {"system_id", dawai::aifr3d_core::contract::kSystemId}});

        while (m_running.load())
        {
            WsFrame frame;
            if (!readWsFrame(*client, frame))
            {
                break;
            }

            if (frame.opcode == 0x8u)
            {
                (void)sendWsFrame(*client, 0x8u, "");
                break;
            }

            if (frame.opcode == 0x9u)
            {
                (void)sendWsFrame(*client, 0xAu, frame.payload);
                continue;
            }

            if (frame.opcode != 0x1u)
            {
                continue;
            }

            const auto parsed = nlohmann::json::parse(frame.payload, nullptr, false);
            std::string prompt;
            std::string analysisDir = "analysis";
            std::string genre = "Auto";
            std::string conversationId = "ws-session";
            std::string requestedModel;
            std::vector<std::filesystem::path> attachments;

            if (parsed.is_object())
            {
                prompt = parsed.value("prompt", "");
                analysisDir = parsed.value("analysis_dir", analysisDir);
                genre = parsed.value("genre", genre);
                conversationId = parsed.value("conversation_id", conversationId);
                requestedModel = parsed.value("model", requestedModel);
                attachments = materializeAttachments(
                    parsed.value("attachments", nlohmann::json::array()), conversationId);
            }
            else
            {
                prompt = frame.payload;
            }

            if (prompt.empty())
            {
                sendJson(nlohmann::json{{"type", "chat.error"}, {"message", "prompt is empty"}});
                continue;
            }

            const dawai::advisory_layer::SessionContext context{
                genre,
                prompt,
                std::string("contract=") + dawai::aifr3d_core::contract::kSystemId +
                    "; metrics=" + dawai::aifr3d_core::contract::kMetricMathPack,
                conversationId,
                prompt,
                requestedModel,
                attachments,
                {},
                {}};

            const auto output = runAdviser(analysisDir, context);
            if (!output)
            {
                sendJson(nlohmann::json{{"type", "chat.error"},
                                        {"message", advisoryUnavailableMessage()}});
                continue;
            }

            sendJson(nlohmann::json{{"type", "chat.reply"},
                                    {"payload",
                                     {{"summary", output->summary30s},
                                      {"summary30s", output->summary30s},
                                      {"details", output->details},
                                     {"safePath", output->safePath},
                                     {"boldPath", output->boldPath},
                                     {"top3Fixes", output->top3Fixes},
                                      {"signal_clarity", output->signalClarity},
                                      {"signal_stability", output->signalClarity},
                                      {"confidence", output->confidence},
                                      {"conversation_id", conversationId}}}});
        }

        client->close();
        return;
    }

    if (request.method == "get" && request.path == "/api/v1/health")
    {
        sendHttpResponse(*client, 200, "OK", doctorReport(m_port).dump(), "application/json");
        client->close();
        return;
    }

    if (request.method == "post" && request.path == "/api/v1/command/run")
    {
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded())
        {
            sendHttpResponse(*client, 400, "Bad Request", R"({"error":"invalid json"})",
                             "application/json");
            client->close();
            return;
        }

        const auto commandLine = body.value("command_line", "");
        const auto result = executeCommandLine(commandLine, m_port);
        sendHttpResponse(*client, result.value("ok", false) ? 200 : 500,
                         result.value("ok", false) ? "OK" : "Command Failed", result.dump(),
                         "application/json");
        client->close();
        return;
    }

    if (request.method == "post" && request.path == "/api/v1/chat/ask")
    {
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded())
        {
            sendHttpResponse(*client, 400, "Bad Request", R"({"error":"invalid json"})",
                             "application/json");
            client->close();
            return;
        }

        const std::string prompt = body.value("prompt", "");
        const std::string analysisDir = body.value("analysis_dir", "analysis");
        const std::string genre = body.value("genre", "Auto");
        const std::string conversationId = body.value("conversation_id", "mobile-session");
        const std::string requestedModel = body.value("model", "");
        const auto attachments = materializeAttachments(
            body.value("attachments", nlohmann::json::array()), conversationId);

        if (prompt.empty())
        {
            sendHttpResponse(*client, 400, "Bad Request", R"({"error":"prompt is empty"})",
                             "application/json");
            client->close();
            return;
        }

        const dawai::advisory_layer::SessionContext context{
            genre,
            prompt,
            std::string("contract=") + dawai::aifr3d_core::contract::kSystemId +
                "; metrics=" + dawai::aifr3d_core::contract::kMetricMathPack,
            conversationId,
            prompt,
            requestedModel,
            attachments,
            {},
            {}};

        const auto output = runAdviser(analysisDir, context);
        nlohmann::json response;
        if (output)
        {
            response = nlohmann::json{{"summary", output->summary30s},
                                      {"summary30s", output->summary30s},
                                      {"details", output->details},
                                      {"safePath", output->safePath},
                                      {"boldPath", output->boldPath},
                                      {"top3Fixes", output->top3Fixes},
                                      {"signal_clarity", output->signalClarity},
                                      {"signal_stability", output->signalClarity},
                                      {"confidence", output->confidence},
                                      {"genre", genre},
                                      {"conversation_id", conversationId}};
        }
        else
        {
            response = unavailableChat(genre, conversationId);
        }

        sendHttpResponse(*client, response.value("ok", true) ? 200 : 503,
                         response.value("ok", true) ? "OK" : "Unavailable", response.dump(),
                         "application/json");
        client->close();
        return;
    }

    if (request.method == "post" && request.path == "/api/v1/files/upload")
    {
        const auto body = nlohmann::json::parse(request.body, nullptr, false);
        if (body.is_discarded())
        {
            sendHttpResponse(*client, 400, "Bad Request", R"({"error":"invalid json"})",
                             "application/json");
            client->close();
            return;
        }

        const auto target = safeToken(body.value("target", "uploads"), "uploads");
        const auto fileName = safeToken(body.value("file_name", "file.bin"), "file");
        const auto bytesBase64 = body.value("bytes_base64", "");

        if (bytesBase64.empty())
        {
            sendHttpResponse(*client, 400, "Bad Request", R"({"error":"bytes_base64 missing"})",
                             "application/json");
            client->close();
            return;
        }

        juce::MemoryBlock decoded;
        juce::MemoryOutputStream decodedStream(decoded, false);
        if (!juce::Base64::convertFromBase64(decodedStream,
                                             juce::String::fromUTF8(bytesBase64.c_str())))
        {
            sendHttpResponse(*client, 400, "Bad Request", R"({"error":"invalid base64 payload"})",
                             "application/json");
            client->close();
            return;
        }

        const auto folder = std::filesystem::path("analysis") / "uploads" / target;
        std::filesystem::create_directories(folder);

        const auto outPath =
            folder / (std::to_string(juce::Time::currentTimeMillis()) + "_" + fileName);
        std::ofstream output(outPath, std::ios::binary);
        output.write(static_cast<const char*>(decoded.getData()),
                     static_cast<std::streamsize>(decoded.getSize()));
        output.close();

        sendHttpResponse(*client, 200, "OK", makeUploadResponse(outPath, target).dump(),
                         "application/json");
        client->close();
        return;
    }

    sendHttpResponse(*client, 404, "Not Found", R"({"error":"route not found"})",
                     "application/json");
    client->close();
}

void DaemonServer::ensureAutoDependencies()
{
    // OpenAI-only advisory path: no local model bootstrap.
}

std::optional<dawai::advisory_layer::AdvisoryOutput>
DaemonServer::runAdviser(const std::filesystem::path& analysisDir,
                         const dawai::advisory_layer::SessionContext& context) const
{
    std::lock_guard<std::mutex> lock(m_adviserMutex);
    if (!m_adviser)
    {
        m_adviser = dawai::advisory_layer::LocalBrainAdviser::createFromEnvironment();
    }
    if (!m_adviser)
    {
        return std::nullopt;
    }

    std::filesystem::create_directories(analysisDir);
    return m_adviser->advise(analysisDir, context);
}

} // namespace dawai
