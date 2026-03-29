#include "DSP/FeatureExtractor.h"
#include "DSP/ScoringEngine.h"
#include "DSP/SpectrumAnalyzerFFT.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <numbers>

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

juce::AudioBuffer<float> makeToneBuffer(const std::array<float, 6>& amplitudes)
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
        buffer.setSample(1, sampleIndex, sample);
    }

    if (peak > 0.98f)
    {
        buffer.applyGain(0.98f / peak);
    }

    return buffer;
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
    signature.presenceBandDb = spectral.highMidBandDb;
    signature.airBandDb = spectral.highBandDb;
    return signature;
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
} // namespace

int main()
{
    using namespace aifred::dsp;

    SpectrumAnalyzerFFT spectrumAnalyzer;
    spectrumAnalyzer.prepare(kSampleRate);

    FeatureExtractor featureExtractor;
    featureExtractor.prepare(kSampleRate, kBlockSize);

    ScoringEngine scoringEngine;

    const auto balancedBuffer = makeToneBuffer({0.11f, 0.12f, 0.10f, 0.09f, 0.08f, 0.07f});
    const auto bassCutBuffer = makeToneBuffer({0.01f, 0.02f, 0.10f, 0.09f, 0.08f, 0.07f});
    const auto lowMidBoostBuffer = makeToneBuffer({0.10f, 0.11f, 0.24f, 0.08f, 0.08f, 0.07f});
    const auto highBoostBuffer = makeToneBuffer({0.11f, 0.12f, 0.09f, 0.08f, 0.18f, 0.22f});

    const auto balancedSpectral = spectrumAnalyzer.analyze(balancedBuffer);
    const auto balancedFeatures = featureExtractor.extract(balancedBuffer, &balancedSpectral);
    const auto referenceSignature = makeSignature(balancedFeatures, balancedSpectral);

    const auto bassCutSpectral = spectrumAnalyzer.analyze(bassCutBuffer);
    const auto bassCutFeatures = featureExtractor.extract(bassCutBuffer, &bassCutSpectral);

    const auto lowMidBoostSpectral = spectrumAnalyzer.analyze(lowMidBoostBuffer);
    const auto lowMidBoostFeatures = featureExtractor.extract(lowMidBoostBuffer, &lowMidBoostSpectral);

    const auto highBoostSpectral = spectrumAnalyzer.analyze(highBoostBuffer);
    const auto highBoostFeatures = featureExtractor.extract(highBoostBuffer, &highBoostSpectral);

    const auto balancedScores =
        scoringEngine.score(balancedFeatures, balancedSpectral, referenceSignature,
                            referenceSignature, balancedFeatures.harshness, balancedFeatures.mud,
                            balancedFeatures.air, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 30.0f);
    const auto bassCutScores =
        scoringEngine.score(bassCutFeatures, bassCutSpectral, referenceSignature,
                            referenceSignature, bassCutFeatures.harshness, bassCutFeatures.mud,
                            bassCutFeatures.air, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 30.0f);
    const auto lowMidBoostScores =
        scoringEngine.score(lowMidBoostFeatures, lowMidBoostSpectral, referenceSignature,
                            referenceSignature, lowMidBoostFeatures.harshness,
                            lowMidBoostFeatures.mud, lowMidBoostFeatures.air, 1.0f, 1.0f, 1.0f,
                            0.0f, 0.0f, 30.0f);
    const auto highBoostScores =
        scoringEngine.score(highBoostFeatures, highBoostSpectral, referenceSignature,
                            referenceSignature, highBoostFeatures.harshness,
                            highBoostFeatures.mud, highBoostFeatures.air, 1.0f, 1.0f, 1.0f,
                            0.0f, 0.0f, 30.0f);

    std::cout << "baseline tone=" << balancedScores.tone
              << " alignment=" << balancedScores.mixAlignment
              << " mud=" << balancedFeatures.mud
              << " harshness=" << balancedFeatures.harshness << std::endl;
    std::cout << "bass_cut tone=" << bassCutScores.tone
              << " alignment=" << bassCutScores.mixAlignment
              << " sub_db=" << bassCutSpectral.subBandDb
              << " low_db=" << bassCutSpectral.lowBandDb << std::endl;
    std::cout << "low_mid_boost tone=" << lowMidBoostScores.tone
              << " alignment=" << lowMidBoostScores.mixAlignment
              << " mud=" << lowMidBoostFeatures.mud
              << " low_mid_db=" << lowMidBoostSpectral.lowMidBandDb << std::endl;
    std::cout << "high_boost tone=" << highBoostScores.tone
              << " alignment=" << highBoostScores.mixAlignment
              << " harshness=" << highBoostFeatures.harshness
              << " high_mid_db=" << highBoostSpectral.highMidBandDb
              << " high_db=" << highBoostSpectral.highBandDb << std::endl;

    bool ok = true;
    ok &= require(bassCutSpectral.subBandDb < balancedSpectral.subBandDb - 8.0f,
                  "bass cut did not lower sub-band energy");
    ok &= require(bassCutSpectral.lowBandDb < balancedSpectral.lowBandDb - 6.0f,
                  "bass cut did not lower low-band energy");
    ok &= require(highBoostSpectral.highMidBandDb > balancedSpectral.highMidBandDb + 4.0f,
                  "high boost did not raise high-mid energy");
    ok &= require(highBoostSpectral.highBandDb > balancedSpectral.highBandDb + 6.0f,
                  "high boost did not raise high-band energy");
    ok &= require(bassCutScores.tone < balancedScores.tone - 0.08f,
                  "bass cut did not lower tone score");
    ok &= require(lowMidBoostSpectral.lowMidBandDb > balancedSpectral.lowMidBandDb + 5.0f,
                  "low-mid boost did not raise low-mid energy");
    ok &= require(lowMidBoostFeatures.mud > balancedFeatures.mud + 0.07f,
                  "low-mid boost did not raise mud behavior");
    ok &= require(highBoostFeatures.harshness > balancedFeatures.harshness + 0.05f,
                  "high boost did not raise harshness behavior");
    ok &= require(lowMidBoostScores.tone < balancedScores.tone - 0.08f,
                  "low-mid boost did not lower tone score");
    ok &= require(highBoostScores.tone < balancedScores.tone - 0.08f,
                  "high boost did not lower tone score");
    ok &= require(bassCutScores.mixAlignment < balancedScores.mixAlignment - 0.05f,
                  "bass cut did not lower mix alignment");
    ok &= require(lowMidBoostScores.mixAlignment < balancedScores.mixAlignment - 0.05f,
                  "low-mid boost did not lower mix alignment");
    ok &= require(highBoostScores.mixAlignment < balancedScores.mixAlignment - 0.05f,
                  "high boost did not lower mix alignment");

    return ok ? 0 : 1;
}
