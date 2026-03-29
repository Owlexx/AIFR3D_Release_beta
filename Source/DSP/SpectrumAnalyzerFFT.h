#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

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
    [[nodiscard]] SpectralFrame analyze(const juce::AudioBuffer<float>& buffer);
    float spectralTiltDbPerOct(const juce::AudioBuffer<float>& buffer);

  private:
    static constexpr int kFftOrder = 12;
    static constexpr int kFftSize = 1 << kFftOrder;

    double m_sampleRate = 48000.0;
    std::size_t m_bandCount = 32;
    juce::dsp::FFT m_fft{kFftOrder};
    std::vector<float> m_fifo;
    std::vector<float> m_window;
    std::vector<float> m_timeDomain;
    std::vector<float> m_fftData;
    int m_fifoWrite = 0;
    int m_fifoCount = 0;
};

} // namespace audiosynth::dsp
