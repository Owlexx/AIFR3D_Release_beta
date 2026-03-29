#pragma once

#include <JuceHeader.h>

#include "SpectrumAnalyzerFFT.h"

#include <deque>

namespace aifred::dsp
{

struct FeatureFrame
{
    float rms = 0.0f;
    float peak = 0.0f;
    float crestDb = 0.0f;
    float midEnergy = 0.0f;
    float sideEnergy = 0.0f;
    float correlation = 0.0f;
    float transientDensity = 0.0f;
    float transientRateHz = 0.0f;
    float harshness = 0.0f;
    float mud = 0.0f;
    float air = 0.0f;
};

class FeatureExtractor
{
  public:
    void prepare(double sampleRate, int maxBlockSize);
    FeatureFrame extract(const juce::AudioBuffer<float>& buffer,
                         const SpectralFrame* spectral = nullptr);

  private:
    double m_sampleRate = 48000.0;
    int m_maxBlockSize = 512;
    double m_processedSamples = 0.0;
    float m_previousMidSample = 0.0f;
    double m_fluxBaseline = 0.0;
    int m_transientCooldownSamples = 0;
    std::deque<double> m_transientEventTimesSec;
};

} // namespace aifred::dsp
