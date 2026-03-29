#pragma once

#include <JuceHeader.h>

#include <deque>
#include <vector>

namespace aifred::dsp
{

struct LoudnessFrame
{
    float momentaryLufs = -70.0f;
    float integratedLufs = -70.0f;
    float shortTermLufs = -70.0f;
    float truePeakDbTP = -120.0f;
};

class LoudnessEBUR128
{
  public:
    struct EnergyBlock
    {
        double sumSq = 0.0;
        int samples = 0;
    };

    void reset(double sampleRate);
    LoudnessFrame process(const juce::AudioBuffer<float>& buffer, double tSec);

  private:
    struct Biquad
    {
        double b0 = 1.0;
        double b1 = 0.0;
        double b2 = 0.0;
        double a1 = 0.0;
        double a2 = 0.0;
        double z1 = 0.0;
        double z2 = 0.0;

        void reset()
        {
            z1 = 0.0;
            z2 = 0.0;
        }

        double process(double input)
        {
            const double output = (b0 * input) + z1;
            z1 = (b1 * input) - (a1 * output) + z2;
            z2 = (b2 * input) - (a2 * output);
            return output;
        }
    };

    struct ChannelState
    {
        Biquad shelf;
        Biquad highPass;
    };

    void designKWeightingFilters();
    [[nodiscard]] double filteredEnergyForWindow(int targetSamples) const;
    [[nodiscard]] float loudnessFromEnergy(double energyPerSample) const;
    [[nodiscard]] float computeIntegratedLufs() const;
    [[nodiscard]] float computeTruePeakDbTP(const juce::AudioBuffer<float>& buffer) const;

    double m_sampleRate = 48000.0;
    std::deque<EnergyBlock> m_recentBlocks;
    int m_recentSamples = 0;
    std::vector<double> m_integratedBlockEnergy;
    std::vector<ChannelState> m_channelStates;
    int m_integratedHopSamplesPending = 0;
    double m_lastProcessTimeSec = -1.0;
    int m_recentChunkRetentionSamples = 0;
};

} // namespace aifred::dsp
