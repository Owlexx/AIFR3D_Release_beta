#include "dawai/aifr3d_core/session_store.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>

namespace dawai::aifr3d_core
{

namespace
{
float clamp01(float value)
{
    return std::clamp(value, 0.0F, 1.0F);
}

std::string nowIsoUtc()
{
    const auto now = std::chrono::system_clock::now();
    const auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buffer;
}

nlohmann::json metricsToJson(const SessionDeviationMetrics& metrics)
{
    return {{"integrated_lufs", metrics.integratedLufs},
            {"true_peak_dbtp", metrics.truePeakDbtp},
            {"crest_factor_db", metrics.crestFactorDb},
            {"spectral_balance", metrics.spectralBalance},
            {"stereo_width", metrics.stereoWidth},
            {"stereo_correlation", metrics.stereoCorrelation},
            {"transient_density", metrics.transientDensity}};
}

SessionDeviationMetrics metricsFromJson(const nlohmann::json& value)
{
    SessionDeviationMetrics out;
    if (!value.is_object())
    {
        return out;
    }
    out.integratedLufs = value.value("integrated_lufs", 0.0);
    out.truePeakDbtp = value.value("true_peak_dbtp", 0.0);
    out.crestFactorDb = value.value("crest_factor_db", 0.0);
    out.spectralBalance = value.value("spectral_balance", 0.0);
    out.stereoWidth = value.value("stereo_width", 0.0);
    out.stereoCorrelation = value.value("stereo_correlation", 0.0);
    out.transientDensity = value.value("transient_density", 0.0);
    return out;
}

} // namespace

SessionStore::SessionStore(std::filesystem::path path, std::size_t maxSessions,
                           std::size_t maxLogBytes, std::size_t maxLogDays)
    : m_path(std::move(path)), m_maxSessions(std::max<std::size_t>(1, maxSessions)),
      m_maxLogBytes(std::max<std::size_t>(8 * 1024, maxLogBytes)),
      m_maxLogDays(std::max<std::size_t>(1, maxLogDays))
{
}

SessionStoreData SessionStore::load() const
{
    SessionStoreData out;
    out.maxSessions = m_maxSessions;
    out.candles.assign(m_maxSessions, SessionCandleRecord{});

    std::ifstream input(m_path);
    if (!input)
    {
        return out;
    }

    nlohmann::json root;
    input >> root;
    if (!root.is_object())
    {
        return out;
    }

    out.schemaVersion = root.value("schema_version", std::string(kSessionStoreSchemaVersion));
    out.firstSessionBaseline = root.value("first_session_baseline", -1.0F);
    out.writeIndex = static_cast<std::size_t>(std::max(0, root.value("write_index", 0)));
    out.writeIndex %= m_maxSessions;

    const auto candles = root.value("candles", nlohmann::json::array());
    if (!candles.is_array())
    {
        return out;
    }

    std::size_t i = 0;
    for (const auto& candle : candles)
    {
        if (i >= m_maxSessions || !candle.is_object())
        {
            break;
        }

        out.candles[i].open = clamp01(candle.value("open", 0.5F));
        out.candles[i].close = clamp01(candle.value("close", 0.5F));
        out.candles[i].high = clamp01(candle.value("high", 0.52F));
        out.candles[i].low = clamp01(candle.value("low", 0.48F));
        out.candles[i].flags = candle.value("flags", static_cast<std::uint32_t>(0));
        out.candles[i].t0 = candle.value("t0", 0.0);
        out.candles[i].t1 = candle.value("t1", 0.0);
        out.candles[i].deviation = metricsFromJson(candle.value("deviation", nlohmann::json::object()));
        ++i;
    }

    return out;
}

bool SessionStore::save(SessionStoreData data) const
{
    if (data.candles.size() > m_maxSessions)
    {
        data.candles.resize(m_maxSessions);
    }
    if (data.candles.size() < m_maxSessions)
    {
        data.candles.resize(m_maxSessions, SessionCandleRecord{});
    }
    data.maxSessions = m_maxSessions;
    data.writeIndex %= m_maxSessions;
    data.schemaVersion = kSessionStoreSchemaVersion;

    nlohmann::json root;
    root["schema_version"] = data.schemaVersion;
    root["max_sessions"] = data.maxSessions;
    root["write_index"] = data.writeIndex;
    root["first_session_baseline"] = data.firstSessionBaseline;
    root["candles"] = nlohmann::json::array();

    for (const auto& candle : data.candles)
    {
        root["candles"].push_back({{"open", candle.open},
                                   {"close", candle.close},
                                   {"high", candle.high},
                                   {"low", candle.low},
                                   {"flags", candle.flags},
                                   {"t0", candle.t0},
                                   {"t1", candle.t1},
                                   {"deviation", metricsToJson(candle.deviation)}});
    }

    std::filesystem::create_directories(m_path.parent_path());
    std::ofstream output(m_path, std::ios::trunc);
    if (!output)
    {
        return false;
    }
    output << root.dump(2);
    return true;
}

std::filesystem::path SessionStore::logPath() const
{
    return m_path.parent_path() / "session_store.log";
}

void SessionStore::rotateLogIfNeeded() const
{
    const auto path = logPath();
    std::error_code ec;
    const auto hasLog = std::filesystem::exists(path, ec);
    if (!hasLog || ec)
    {
        return;
    }

    const auto size = std::filesystem::file_size(path, ec);
    const auto age = std::filesystem::last_write_time(path, ec);
    bool rotate = (!ec && size > m_maxLogBytes);

    if (!rotate && !ec)
    {
        const auto now = std::filesystem::file_time_type::clock::now();
        const auto ageHours =
            std::chrono::duration_cast<std::chrono::hours>(now - age).count();
        rotate = ageHours > static_cast<long long>(m_maxLogDays * 24);
    }

    if (!rotate)
    {
        return;
    }

    const auto rotated = path.string() + ".1";
    std::filesystem::remove(rotated, ec);
    std::filesystem::rename(path, rotated, ec);
}

void SessionStore::appendLogLine(const std::string& line) const
{
    rotateLogIfNeeded();
    const auto path = logPath();
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::app);
    if (!output)
    {
        return;
    }
    output << nowIsoUtc() << " " << line << "\n";
}

} // namespace dawai::aifr3d_core
