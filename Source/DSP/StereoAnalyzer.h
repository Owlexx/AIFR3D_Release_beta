#pragma once

#include "FeatureExtractor.h"

namespace aifred::dsp
{

struct StereoFieldFrame
{
    float wAxis = 0.5f;
    float xAxis = 0.5f;
    float yAxis = 0.5f;
    float zAxis = 0.0f;
    float width = 0.0f;
    float correlation = 1.0f;
    float leftEnergy = 0.5f;
    float rightEnergy = 0.5f;
    float midEnergy = 0.5f;
    float sideEnergy = 0.0f;
    float centerDominance = 0.5f;
    float sideDominance = 0.0f;
    float stereoMotion = 0.0f;
    float spatialSpread = 0.0f;
    float subMonoIntegrity = 1.0f;
    float phaseRisk = 0.0f;
    float lowBandCorrelation = 1.0f;
    float lowBandPhaseRisk = 0.0f;
};

class StereoAnalyzer
{
  public:
    void prepare(double sampleRate) noexcept;
    [[nodiscard]] StereoFieldFrame analyze(const juce::AudioBuffer<float>& buffer);
    float stereoSignatureMatch(const FeatureFrame& user, const FeatureFrame& reference) const;
    float phaseRisk(const FeatureFrame& f) const;
    float subMonoIntegrity(const FeatureFrame& f) const;

  private:
    double m_sampleRate = 48000.0;
    StereoFieldFrame m_previousField;
    bool m_hasPreviousField = false;
};

} // namespace aifred::dsp
