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

namespace audiosynth::dsp
{

class AnalysisEngine
{
  public:
    void prepare(double sampleRate, int blockSize);
    bool process(const juce::AudioBuffer<float>& buffer, double tSec, AnalysisFrame& out);

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
};

} // namespace audiosynth::dsp
