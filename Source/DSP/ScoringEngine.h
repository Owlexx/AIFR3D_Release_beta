#pragma once

#include "ReferenceModel.h"

namespace audiosynth::dsp
{

struct SegmentScores
{
    float tone = 0.0f;
    float dynamics = 0.0f;
    float space = 0.0f;
    float punch = 0.0f;
    float balance = 0.0f;
    float behaviorIndex = 0.0f;
    float signalClarity = 0.0f;
    float mixAlignment = 0.0f;

    // Deprecated aliases retained for compatibility.
    float life = 0.0f;
    float signalStability = 0.0f;
    float confidence = 0.0f;
    float approval = 0.0f;
};

class ScoringEngine
{
  public:
    SegmentScores score(const FeatureFrame& feature, const SpectralFrame& spectral,
                        const ReferenceSignature& referenceMatch,
                        const ReferenceSignature& genreMean, float harshness, float mud, float air,
                        float transientMatch, float stereoMatch, float subMonoIntegrity,
                        float phaseRisk, float compressionRisk, float playbackSeconds) const;
};

} // namespace audiosynth::dsp
