#pragma once

#include "DynamicsTracker.h"
#include "FeatureExtractor.h"
#include "LoudnessEBUR128.h"
#include "ReferenceModel.h"
#include "ScoringEngine.h"
#include "SnapshotManager.h"
#include "SpectrumAnalyzerFFT.h"
#include "StereoAnalyzer.h"
#include "TransientDetector.h"

#include "../Common/AnalysisTypes.h"

#include <deque>

namespace audiosynth::dsp
{

struct VolatilityPoint
{
    double tSec = 0.0;
    float momentaryLufs = -70.0f;
    float shortTermLufs = -70.0f;
    float crestDb = 0.0f;
    float spectralTiltDb = 0.0f;
    float width = 0.0f;
    float correlation = 1.0f;
    float transientDensity = 0.0f;
    float truePeakDbTP = -120.0f;
};

class AnalysisEngine
{
  public:
    void prepare(double sampleRate, int blockSize);
    bool process(const juce::AudioBuffer<float>& buffer, double tSec, AnalysisFrame& out);
    void setPinnedGenre(GenreId genre) noexcept
    {
        m_pinnedGenre = genre;
    }

  private:
    FeatureExtractor m_featureExtractor;
    LoudnessEBUR128 m_loudness;
    SpectrumAnalyzerFFT m_spectrum;
    DynamicsTracker m_dynamics;
    TransientDetector m_transient;
    StereoAnalyzer m_stereo;
    ReferenceModel m_reference;
    ScoringEngine m_scoring;
    SnapshotManager m_snapshots;

    GenreId m_lastGenre = GenreId::Unknown;
    float m_lastGenreConfidence = 0.0f;
    GenreId m_pinnedGenre = GenreId::Unknown;
    int m_activeSignalFrames = 0;
    juce::AudioBuffer<float> m_measurementBuffer;
    std::deque<VolatilityPoint> m_volatilityHistory;
    int m_sessionSamples = 0;
    float m_sessionShortTermLufsSum = 0.0f;
    float m_sessionCrestDbSum = 0.0f;
    float m_sessionWidthSum = 0.0f;
    float m_sessionCorrelationSum = 0.0f;
    float m_sessionTransientDensitySum = 0.0f;
    float m_sessionSpectralTiltSum = 0.0f;
};

} // namespace audiosynth::dsp
