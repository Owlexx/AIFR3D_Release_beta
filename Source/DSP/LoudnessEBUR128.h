#pragma once

#include <JuceHeader.h>

#include <deque>
#include <memory>
#include <vector>

namespace audiosynth::dsp
{

struct LoudnessFrame
{
    float integratedLufs = -70.0f;
    float shortTermLufs = -70.0f;
    float momentaryLufs = -70.0f;
    float truePeakDbTP = -120.0f;
    float peakDbFS = -120.0f;
};

class LoudnessEBUR128
{
  public:
    struct EnergyBlock
    {
        double sumSq = 0.0;
        int samples = 0;
    };

    struct BiquadCoefficients
    {
        double b0 = 1.0;
        double b1 = 0.0;
        double b2 = 0.0;
        double a1 = 0.0;
        double a2 = 0.0;
    };

    struct BiquadState
    {
        double x1 = 0.0;
        double x2 = 0.0;
        double y1 = 0.0;
        double y2 = 0.0;

        void reset() noexcept;
        float process(float sample, const BiquadCoefficients& coefficients) noexcept;
    };

    struct LoudnessBlock
    {
        double energyPerSample = 0.0;
        float loudnessLufs = -70.0f;
    };

    void reset(double sampleRate, int maxBlockSize, int maxChannels = 2);
    LoudnessFrame process(const juce::AudioBuffer<float>& buffer);

  private:
    void updateWeightingCoefficients();
    void ensureChannelState(int channels);
    [[nodiscard]] double computeTruePeakDbTp(const juce::AudioBuffer<float>& buffer);
    [[nodiscard]] float computeIntegratedLufs() const;
    [[nodiscard]] float computeWindowLufs(std::size_t steps) const;

    double m_sampleRate = 48000.0;
    int m_maxBlockSize = 0;
    int m_stepSamples = 4800;
    std::deque<EnergyBlock> m_stepBlocks;
    std::deque<LoudnessBlock> m_integratedBlocks;
    double m_stepAccumulatorEnergy = 0.0;
    int m_stepAccumulatorSamples = 0;
    BiquadCoefficients m_shelfCoefficients;
    BiquadCoefficients m_highPassCoefficients;
    std::vector<BiquadState> m_shelfStates;
    std::vector<BiquadState> m_highPassStates;
    juce::AudioBuffer<float> m_truePeakScratch;
    std::unique_ptr<juce::dsp::Oversampling<float>> m_truePeakOversampler;
};

} // namespace audiosynth::dsp
