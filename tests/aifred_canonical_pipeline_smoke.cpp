#include "Common/MixMetrics.h"
#include "DSP/AnalysisEngine.h"
#include "DSP/FeatureExtractor.h"
#include "DSP/ReferenceModel.h"
#include "DSP/ScoringEngine.h"
#include "DSP/SpectrumAnalyzerFFT.h"
#include "Interpretation/DiagnosticEngine.h"
#include "UI/UiModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>

#include <nlohmann/json.hpp>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 4096;
constexpr std::array<std::array<float, 4>, 6> kBandVoicesHz = {{
    {{28.0f, 41.0f, 55.0f, 0.0f}},
    {{72.0f, 92.0f, 110.0f, 0.0f}},
    {{160.0f, 240.0f, 320.0f, 380.0f}},
    {{520.0f, 900.0f, 1300.0f, 1800.0f}},
    {{2400.0f, 3400.0f, 4700.0f, 5800.0f}},
    {{7000.0f, 9000.0f, 10500.0f, 11800.0f}},
}};

juce::AudioBuffer<float> makeToneBuffer(const std::array<float, 6>& amplitudes,
                                        float rightChannelScale = 1.0f)
{
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    float peak = 0.0f;
    for (int sampleIndex = 0; sampleIndex < kBlockSize; ++sampleIndex)
    {
        const float time = static_cast<float>(sampleIndex / kSampleRate);
        float sample = 0.0f;
        for (std::size_t band = 0; band < amplitudes.size(); ++band)
        {
            int voiceCount = 0;
            for (float hz : kBandVoicesHz[band])
            {
                if (hz <= 0.0f)
                {
                    continue;
                }

                const float phase =
                    0.31f * static_cast<float>(band + static_cast<std::size_t>(voiceCount) + 1U);
                sample += (amplitudes[band] / 4.0f) *
                          std::sin(2.0f * std::numbers::pi_v<float> * hz * time + phase);
                ++voiceCount;
            }
        }

        peak = std::max(peak, std::abs(sample));
        buffer.setSample(0, sampleIndex, sample);
        buffer.setSample(1, sampleIndex, sample * rightChannelScale);
    }

    if (peak > 0.98f)
    {
        buffer.applyGain(0.98f / peak);
    }

    return buffer;
}

bool require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

bool requireFinite01(float value, const char* label)
{
    if (!std::isfinite(value) || value < 0.0f || value > 1.0f)
    {
        std::cerr << label << " out of range: " << value << std::endl;
        return false;
    }
    return true;
}

bool almostEqual(float lhs, float rhs, float epsilon = 1.0e-4f)
{
    return std::abs(lhs - rhs) <= epsilon;
}

aifred::dsp::ReferenceSignature makeSignature(const aifred::dsp::FeatureFrame& feature,
                                              const aifred::dsp::SpectralFrame& spectral)
{
    aifred::dsp::ReferenceSignature signature;
    signature.feature = feature;
    signature.spectrumBandsDb = spectral.averagedBinsDb;
    signature.spectralTilt = spectral.spectralTiltDb;
    signature.subBandDb = spectral.subBandDb;
    signature.lowBandDb = spectral.lowBandDb;
    signature.lowMidBandDb = spectral.lowMidBandDb;
    signature.midBandDb = spectral.midBandDb;
    signature.highMidBandDb = spectral.highMidBandDb;
    signature.highBandDb = spectral.highBandDb;
    signature.presenceBandDb = spectral.presenceBandDb;
    signature.airBandDb = spectral.airBandDb;
    signature.truePeakDbTP = -1.0f;
    signature.frequencyBalance = {
        0.08f, 0.22f, 0.20f, 0.18f, 0.16f, 0.10f, 0.06f,
    };
    return signature;
}

std::filesystem::path makeTempRoot(const char* stem)
{
    const auto root = std::filesystem::temp_directory_path() /
                      std::filesystem::path(std::string("aifred_") + stem);
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root / "profiles");
    return root;
}

void writeJsonFile(const std::filesystem::path& path, const nlohmann::json& payload)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    output << payload.dump(2);
}

} // namespace

int main()
{
    using namespace aifred::dsp;

    bool ok = true;
    std::filesystem::current_path(std::filesystem::path(DAWAI_SOURCE_DIR));

    ReferenceSignature emptySignature;
    ok &= require(almostEqual(emptySignature.subBandDb, -90.0f),
                  "default subBandDb must be -90 dB");
    ok &= require(almostEqual(emptySignature.lowBandDb, -90.0f),
                  "default lowBandDb must be -90 dB");
    ok &= require(almostEqual(emptySignature.lowMidBandDb, -90.0f),
                  "default lowMidBandDb must be -90 dB");
    ok &= require(almostEqual(emptySignature.midBandDb, -90.0f),
                  "default midBandDb must be -90 dB");
    ok &= require(almostEqual(emptySignature.highMidBandDb, -90.0f),
                  "default highMidBandDb must be -90 dB");
    ok &= require(almostEqual(emptySignature.highBandDb, -90.0f),
                  "default highBandDb must be -90 dB");
    ok &= require(almostEqual(emptySignature.presenceBandDb, -90.0f),
                  "default presenceBandDb must be -90 dB");
    ok &= require(almostEqual(emptySignature.airBandDb, -90.0f),
                  "default airBandDb must be -90 dB");

    SpectrumAnalyzerFFT spectrumAnalyzer;
    spectrumAnalyzer.prepare(kSampleRate);
    FeatureExtractor featureExtractor;
    featureExtractor.prepare(kSampleRate, kBlockSize);
    ScoringEngine scoringEngine;

    const auto balancedBuffer = makeToneBuffer({0.11f, 0.12f, 0.10f, 0.09f, 0.08f, 0.07f});
    const auto balancedSpectral = spectrumAnalyzer.analyze(balancedBuffer);
    const auto balancedFeatures = featureExtractor.extract(balancedBuffer, &balancedSpectral);
    const auto genreMean = makeSignature(balancedFeatures, balancedSpectral);

    const auto fallbackScores =
        scoringEngine.score(balancedFeatures, balancedSpectral, ReferenceSignature{}, genreMean,
                            balancedFeatures.harshness, balancedFeatures.mud,
                            balancedFeatures.air, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 30.0f);
    const auto missingScores =
        scoringEngine.score(balancedFeatures, balancedSpectral, ReferenceSignature{},
                            ReferenceSignature{}, balancedFeatures.harshness,
                            balancedFeatures.mud, balancedFeatures.air, 1.0f, 1.0f, 1.0f, 0.0f,
                            0.0f, 30.0f);

    ok &= require(fallbackScores.mixAlignment > 0.65f,
                  "genre fallback did not preserve a usable mix alignment");
    ok &= requireFinite01(fallbackScores.tone, "fallback tone");
    ok &= requireFinite01(fallbackScores.dynamics, "fallback dynamics");
    ok &= requireFinite01(fallbackScores.space, "fallback space");
    ok &= requireFinite01(fallbackScores.punch, "fallback punch");
    ok &= requireFinite01(fallbackScores.balance, "fallback balance");
    ok &= requireFinite01(fallbackScores.behaviorIndex, "fallback behaviorIndex");
    ok &= requireFinite01(fallbackScores.signalClarity, "fallback signalClarity");
    ok &= requireFinite01(fallbackScores.mixAlignment, "fallback mixAlignment");
    ok &= require(std::isfinite(missingScores.mixAlignment),
                  "missing reference path produced a non-finite score");
    ok &= require(missingScores.mixAlignment < 0.95f,
                  "missing reference path still looked fake-perfect");
    ok &= require(missingScores.mixAlignment < fallbackScores.mixAlignment,
                  "missing reference path did not score below the genre fallback path");

    {
        ReferenceModel contaminatedModel;
        const auto root = makeTempRoot("contaminated_reference_pool");
        writeJsonFile(root / "profiles" / "bad.json",
                      {{"spectrumBandsDb", {-24.0, -23.0, -22.0, -21.0, -20.0, -19.0, -18.0, -17.0}},
                       {"crestFactorDb", 9.0},
                       {"transientDensity", 0.42},
                       {"midEnergy", 0.70},
                       {"sideEnergy", 0.30},
                       {"correlation", 0.22},
                       {"subBandDb", -22.0},
                       {"lowBandDb", -18.0},
                       {"lowMidBandDb", -16.0},
                       {"midBandDb", -15.0},
                       {"highMidBandDb", -14.0},
                       {"highBandDb", -15.0},
                       {"presenceBandDb", -14.0},
                       {"airBandDb", -15.0}});
        writeJsonFile(root / "pool_manifest.json",
                      {{"schema_version", "refpool-2.2.4"},
                       {"pool_id", "contaminated"},
                       {"references",
                        {{{"reference_id", "bad_ref"},
                          {"genre", "pop"},
                          {"title", "Bad Ref"},
                          {"source_path", "apps/website/assets/audio/catalog/bad.wav"},
                          {"profile_path", "profiles/bad.json"}}}}});

        const bool loaded = contaminatedModel.loadCanonicalPool(root / "pool_manifest.json");
        ok &= require(!loaded, "contaminated reference manifest should not load");
        ok &= require(contaminatedModel.referenceBlocked(),
                      "contaminated reference manifest was not blocked");
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }

    {
        ReferenceModel averagedModel;
        const auto root = makeTempRoot("canonical_pool_average");
        writeJsonFile(root / "profiles" / "ref_a.json",
                      {{"spectrumBandsDb", {-25.0, -22.0, -19.0, -18.0, -17.0, -16.0, -17.0, -18.0}},
                       {"crestFactorDb", 10.0},
                       {"transientDensity", 0.48},
                       {"midEnergy", 0.72},
                       {"sideEnergy", 0.28},
                       {"correlation", 0.18},
                       {"subBandDb", -25.0},
                       {"lowBandDb", -22.0},
                       {"lowMidBandDb", -19.0},
                       {"midBandDb", -18.0},
                       {"highMidBandDb", -17.0},
                       {"highBandDb", -16.0},
                       {"presenceBandDb", -17.0},
                       {"airBandDb", -18.0}});
        writeJsonFile(root / "profiles" / "ref_b.json",
                      {{"spectrumBandsDb", {-23.0, -20.0, -18.0, -17.0, -16.0, -15.0, -16.0, -17.0}},
                       {"subBandDb", -23.0},
                       {"lowBandDb", -20.0},
                       {"lowMidBandDb", -18.0},
                       {"midBandDb", -17.0},
                       {"highMidBandDb", -16.0},
                       {"highBandDb", -15.0},
                       {"presenceBandDb", -16.0},
                       {"airBandDb", -17.0}});
        writeJsonFile(root / "pool_manifest.json",
                      {{"schema_version", "refpool-2.2.4"},
                       {"pool_id", "canonical_test"},
                       {"references",
                        {{{"reference_id", "ref_a"},
                          {"genre", "pop"},
                          {"title", "Ref A"},
                          {"source_path", "assets/reference_intake/licensed_audio/pop/ref_a.wav"},
                          {"profile_path", "profiles/ref_a.json"}},
                         {{"reference_id", "ref_b"},
                          {"genre", "pop"},
                          {"title", "Ref B"},
                          {"source_path", "assets/reference_intake/licensed_audio/pop/ref_b.wav"},
                          {"profile_path", "profiles/ref_b.json"}}}}});

        ok &= require(averagedModel.loadCanonicalPool(root / "pool_manifest.json"),
                      "canonical test manifest failed to load");
        ok &= require(averagedModel.referenceReady(), "canonical test manifest did not become ready");
        float confidence = 0.0f;
        const auto matched = averagedModel.matchGenre(GenreId::Pop, balancedFeatures,
                                                      balancedSpectral, confidence);
        ok &= require(std::abs(matched.mean.feature.crestDb - 10.0f) < 0.01f,
                      "missing crest data polluted averaged crest target");
        ok &= require(std::abs(matched.mean.lowBandDb - (-21.0f)) < 0.01f,
                      "averaged low band target was not computed from usable references");
        ok &= require(std::abs(matched.mean.subBandDb - (-24.0f)) < 0.01f,
                      "averaged sub band target was not computed from usable references");
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }

    {
        const auto canonicalManifest =
            (std::filesystem::path(DAWAI_SOURCE_DIR) / "assets/reference_pools/canonical_25x5/pool_manifest.json").string();
#if defined(_WIN32)
        _putenv_s("DAWAI_REFERENCE_POOL_MANIFEST", canonicalManifest.c_str());
#else
        ::setenv("DAWAI_REFERENCE_POOL_MANIFEST", canonicalManifest.c_str(), 1);
#endif

        AnalysisEngine engine;
        engine.prepare(kSampleRate, kBlockSize);
        engine.setPinnedGenre(GenreId::Rap);

        UiModel ui;
        ui.prepare(30.0);

        juce::AudioBuffer<float> silentBuffer(2, kBlockSize);
        silentBuffer.clear();
        AnalysisFrame frame;

        ok &= require(engine.process(silentBuffer, 0.0, frame), "idle analysis process failed");
        ok &= require(frame.analysisStateCode == kAnalysisStateIdle,
                      "idle input did not report idle state");
        ui.updateFrom(frame);
        ui.advance();
        ok &= require(ui.statusText == "No valid program material",
                      "idle UI state was not truthful");
        ok &= require(ui.visualPulse < 0.03f,
                      "idle UI pulse should settle instead of pretending analysis is active");
        ok &= require(!ui.captureCompareSnapshotA("Mix A"),
                      "idle compare capture should fail without live program material");
        const int initialCandleWrite = ui.candleWrite;

        for (int i = 1; i <= 5; ++i)
        {
            ok &= require(engine.process(balancedBuffer, 0.25 * static_cast<double>(i), frame),
                          "buffer-warming analysis process failed");
        }
        if (frame.analysisStateCode != kAnalysisStateBufferWarming)
        {
            std::cerr << "warm state=" << static_cast<int>(frame.analysisStateCode)
                      << " ref=" << static_cast<int>(frame.referenceStateCode) << std::endl;
            ok = false;
        }
        ui.updateFrom(frame);
        ui.advance();
        if (ui.statusText != "Analysis window warming")
        {
            std::cerr << "warm status=" << ui.statusText << std::endl;
            ok = false;
        }
        ok &= require(ui.visualPulse > 0.04f && ui.visualPulse < 0.20f,
                      "warming UI pulse should stay calm but visibly active");
        ok &= require(ui.candleWrite == initialCandleWrite,
                      "non-scored frames should not write candle history");

        for (int i = 6; i <= 19; ++i)
        {
            ok &= require(engine.process(balancedBuffer, 0.25 * static_cast<double>(i), frame),
                          "active-live analysis process failed");
        }
        if (frame.analysisStateCode != kAnalysisStateActiveLive)
        {
            std::cerr << "live state=" << static_cast<int>(frame.analysisStateCode)
                      << " ref=" << static_cast<int>(frame.referenceStateCode) << std::endl;
            ok = false;
        }

        const auto bassCutBuffer =
            makeToneBuffer({0.01f, 0.02f, 0.10f, 0.09f, 0.08f, 0.07f}, 0.72f);
        for (int i = 20; i <= 28; ++i)
        {
            ok &= require(engine.process(bassCutBuffer, 0.25 * static_cast<double>(i), frame),
                          "valid-scored analysis process failed");
        }
        if (frame.analysisStateCode != kAnalysisStateValidScored)
        {
            std::cerr << "scored state=" << static_cast<int>(frame.analysisStateCode)
                      << " ref=" << static_cast<int>(frame.referenceStateCode)
                      << " scored=" << static_cast<int>(frame.scoredResultValid) << std::endl;
            ok = false;
        }
        ok &= require(frame.scoredResultValid == 1, "scored frame did not mark scores valid");

        auto scoredMetrics = MixMetrics::fromAnalysisFrame(frame);
        const auto scoredReport = DiagnosticEngine::interpret(scoredMetrics);
        ok &= require(scoredReport.analysisState == "valid-scored",
                      "diagnostic report did not preserve valid-scored state");
        ok &= require(!scoredReport.issues.empty(),
                      "scored mismatch did not generate diagnostic issues");

        for (int i = 0; i < 3; ++i)
        {
            ui.updateFrom(scoredMetrics, scoredReport);
        }
        for (int i = 0; i < 6; ++i)
        {
            ui.advance();
        }
        ok &= require(!ui.fixList.empty(), "UI fix list did not adopt diagnostic engine issues");
        ok &= require(ui.fixList.size() == ui.diagnosticFixList.size(),
                      "UI fix list diverged from diagnostic engine issue count");
        ok &= require(ui.fixList.front().title == ui.diagnosticFixList.front().title,
                      "UI fix list is not reading the diagnostic engine output");
        ok &= require(ui.visualPulse > 0.05f,
                      "scored UI pulse did not react to live measured program material");
        ok &= require(ui.captureCompareSnapshotA("Mix A"),
                      "compare Mix A capture failed with valid live program material");
        ok &= require(ui.compareMixA.valid && ui.compareMixA.label == "Mix A",
                      "compare Mix A snapshot did not persist the requested label");

        auto compareMetrics = scoredMetrics;
        compareMetrics.tSec += 1.5;
        compareMetrics.loudness.integratedLufs -= 0.8f;
        compareMetrics.stereo.width += 0.06f;
        ui.updateFrom(compareMetrics, DiagnosticEngine::interpret(compareMetrics));
        ok &= require(ui.captureCompareSnapshotB("Mix B"),
                      "compare Mix B capture failed with valid live program material");
        ok &= require(ui.compareMixB.valid && ui.compareMixB.label == "Mix B",
                      "compare Mix B snapshot did not persist the requested label");

        const int scoredCandleWrite = ui.candleWrite;
        for (double tSec : {30.0, 34.0, 38.0, 42.0, 46.0, 50.0, 54.0, 58.0})
        {
            scoredMetrics.tSec = tSec;
            ui.updateFrom(scoredMetrics, DiagnosticEngine::interpret(scoredMetrics));
        }

        auto quietMetrics = scoredMetrics;
        quietMetrics.tSec = 61.0;
        quietMetrics.loudness.shortTermLufs = -80.0f;
        quietMetrics.dynamics.transientDensity = 0.0f;
        ui.updateFrom(quietMetrics, DiagnosticEngine::interpret(quietMetrics));
        if (ui.candleWrite == scoredCandleWrite)
        {
            std::cerr << "candle write did not advance from " << scoredCandleWrite << std::endl;
            ok = false;
        }
    }

    return ok ? 0 : 1;
}
