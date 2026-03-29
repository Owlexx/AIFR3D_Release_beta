#pragma once

#include "FeatureExtractor.h"
#include "SpectrumAnalyzerFFT.h"

#include "../Common/AnalysisTypes.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace audiosynth::dsp
{

enum class GenreId : uint8_t
{
    Unknown = 0,
    Pop = 1,
    Edm = 2,
    HipHop = 3,
    Rap = 4,
    Dubstep = 5,
    Rock = 6
};

struct FrequencyBalanceTarget
{
    float sub = -90.0f;
    float bass = -90.0f;
    float lowMid = -90.0f;
    float mid = -90.0f;
    float highMid = -90.0f;
    float presence = -90.0f;
    float air = -90.0f;
};

struct FrequencyBalanceClassification
{
    uint8_t statusCode = kFrequencyBalanceStatusNone;
    uint8_t severity = 0;
    std::string advisoryKey;
};

struct LoudnessClassification
{
    uint8_t statusCode = kIntegratedLoudnessStatusNone;
    uint8_t severity = 0;
    float referenceLufs = -14.0f;
    std::string advisoryKey;
};

struct DynamicRangeClassification
{
    uint8_t statusCode = kDynamicRangeStatusNone;
    uint8_t severity = 0;
    float referenceDb = 8.0f;
    std::string advisoryKey;
};

enum class FrequencyBandId : uint8_t
{
    Sub = 0,
    Bass = 1,
    LowMid = 2,
    Mid = 3,
    HighMid = 4,
    Presence = 5,
    Air = 6
};

struct FrequencyBalanceRule
{
    FrequencyBandId band = FrequencyBandId::Sub;
    bool greaterThan = true;
    float threshold = 0.0f;
    uint8_t statusCode = kFrequencyBalanceStatusNone;
    uint8_t severity = 0;
    std::string advisoryKey;
};

struct ReferenceSignature
{
    FeatureFrame feature;
    std::vector<float> spectrumBandsDb;
    FrequencyBalanceTarget frequencyBalance;
    float spectralTilt = -90.0f;
    float subBandDb = -90.0f;
    float truePeakDbTP = -1.0f;
    float lowBandDb = -90.0f;
    float lowMidBandDb = -90.0f;
    float midBandDb = -90.0f;
    float highMidBandDb = -90.0f;
    float highBandDb = -90.0f;
    float presenceBandDb = -90.0f;
    float airBandDb = -90.0f;
};

struct GenreProfile
{
    GenreId id = GenreId::Unknown;
    const char* name = "Unknown";
    ReferenceSignature mean;
    ReferenceSignature match;
};

struct GenreProfileSet
{
    GenreProfile profile;
    std::vector<ReferenceSignature> references;
};

class ReferenceModel
{
  public:
    ReferenceModel();

    [[nodiscard]] const GenreProfile& activeProfile() const noexcept
    {
        return m_active;
    }
    [[nodiscard]] GenreProfile detectGenre(const FeatureFrame& feature, const SpectralFrame& spectral,
                                           float& outConfidence) const;
    void setActiveProfile(const GenreProfile& profile)
    {
        m_active = profile;
    }
    bool loadCanonicalPool(const std::filesystem::path& manifestPath);
    bool loadMetricConfig(const std::filesystem::path& configPath);
    [[nodiscard]] FrequencyBalanceClassification
    classifyFrequencyBalance(const FrequencyBalanceTarget& balance) const;
    [[nodiscard]] LoudnessClassification classifyIntegratedLoudness(float integratedLufs) const;
    [[nodiscard]] DynamicRangeClassification classifyDynamicRange(float dynamicRangeDb) const;
    [[nodiscard]] GenreProfile matchGenre(GenreId id, const FeatureFrame& feature,
                                          const SpectralFrame& spectral,
                                          float& outConfidence) const;
    [[nodiscard]] bool referenceReady() const noexcept
    {
        return m_referencePoolReady;
    }
    [[nodiscard]] bool referenceBlocked() const noexcept
    {
        return m_referencePoolBlocked;
    }
    [[nodiscard]] bool activeProfileUsable() const noexcept;
    [[nodiscard]] const std::filesystem::path& manifestPath() const noexcept
    {
        return m_manifestPath;
    }

    [[nodiscard]] static const char* genreName(GenreId id) noexcept;

  private:
    static bool hasUsableSpectralValue(float value);
    static bool hasUsableFrequencyEnergy(float value);
    static bool signatureHasUsableFeatureData(const ReferenceSignature& signature);
    static bool signatureHasUsableSpectralData(const ReferenceSignature& signature);
    static bool signatureUsable(const ReferenceSignature& signature);
    static float distanceToProfile(const FeatureFrame& feature, const SpectralFrame& spectral,
                                   const ReferenceSignature& signature);

    std::array<GenreProfileSet, 6> m_profiles{};
    GenreProfile m_active;
    GenreProfile m_unknown;
    std::vector<FrequencyBalanceRule> m_frequencyBalanceRules;
    float m_integratedLoudnessReferenceLufs = -14.0f;
    std::array<float, 6> m_dynamicRangeTargets{{7.0f, 5.0f, 7.0f, 8.0f, 5.0f, 8.0f}};
    bool m_referencePoolReady = false;
    bool m_referencePoolBlocked = false;
    std::filesystem::path m_manifestPath;
};

} // namespace audiosynth::dsp
