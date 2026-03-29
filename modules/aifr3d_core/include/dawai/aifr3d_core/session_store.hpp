#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dawai::aifr3d_core
{

inline constexpr const char* kSessionStoreSchemaVersion = "session_store_v1";

struct SessionDeviationMetrics
{
    double integratedLufs = 0.0;
    double truePeakDbtp = 0.0;
    double crestFactorDb = 0.0;
    double spectralBalance = 0.0;
    double stereoWidth = 0.0;
    double stereoCorrelation = 0.0;
    double transientDensity = 0.0;
};

struct SessionCandleRecord
{
    float open = 0.5F;
    float close = 0.5F;
    float high = 0.52F;
    float low = 0.48F;
    std::uint32_t flags = 0;
    double t0 = 0.0;
    double t1 = 0.0;
    SessionDeviationMetrics deviation;
};

struct SessionStoreData
{
    std::string schemaVersion = kSessionStoreSchemaVersion;
    std::size_t maxSessions = 10;
    std::size_t writeIndex = 0;
    float firstSessionBaseline = -1.0F;
    std::vector<SessionCandleRecord> candles;
};

class SessionStore
{
  public:
    explicit SessionStore(std::filesystem::path path, std::size_t maxSessions = 10,
                          std::size_t maxLogBytes = 256 * 1024, std::size_t maxLogDays = 14);

    [[nodiscard]] SessionStoreData load() const;
    bool save(SessionStoreData data) const;
    void appendLogLine(const std::string& line) const;

  private:
    void rotateLogIfNeeded() const;
    [[nodiscard]] std::filesystem::path logPath() const;

    std::filesystem::path m_path;
    std::size_t m_maxSessions;
    std::size_t m_maxLogBytes;
    std::size_t m_maxLogDays;
};

} // namespace dawai::aifr3d_core
