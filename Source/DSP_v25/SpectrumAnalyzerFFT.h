#pragma once

#include <JuceHeader.h>

#include <vector>

namespace audiosynth::dsp
{

struct SpectralFrame
{
    std::vector<float> averagedBinsDb;
    float binWidthHz = 0.0f;
    float subBandDb = -90.0f;
    float spectralTiltDb = 0.0f;
    float lowBandDb = -90.0f;
    float lowMidBandDb = -90.0f;
    float midBandDb = -90.0f;
    float highMidBandDb = -90.0f;
    float highBandDb = -90.0f;
    float presenceBandDb = -90.0f;
    float airBandDb = -90.0f;
};

class SpectrumAnalyzerFFT
{
  public:
    void prepare(double sampleRate);
    [[nodiscard]] SpectralFrame analyze(const juce::AudioBuffer<float>& buffer) const;
    float spectralTiltDbPerOct(const juce::AudioBuffer<float>& buffer) const;

  private:
    double m_sampleRate = 48000.0;
    std::size_t m_bandCount = 32;
};

} // namespace audiosynth::dsp
