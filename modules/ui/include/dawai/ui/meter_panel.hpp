#pragma once

#include "dawai/metering/metering_engine.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <optional>

namespace dawai::ui
{

class MeterPanel : public juce::Component
{
  public:
    enum class Mode
    {
        Analyze,
        Compare,
        Reference
    };

    void setMode(Mode mode);
    void setSnapshot(const dawai::metering::MeterSnapshot& snapshot);
    void captureCompareMixA();
    void captureCompareMixB();
    void clearCompareMixB();
    void setReferenceTruePeakDbtp(double value);
    void setAnimationPhase(float phase, float pulse);

    void paint(juce::Graphics& g) override;

  private:
    Mode m_mode = Mode::Analyze;
    dawai::metering::MeterSnapshot m_snapshot;
    std::optional<dawai::metering::MeterSnapshot> m_compareMixA;
    std::optional<dawai::metering::MeterSnapshot> m_compareMixB;
    double m_referenceTruePeakDbtp = -1.0;
    float m_animationPhase = 0.0F;
    float m_animationPulse = 0.0F;
};

} // namespace dawai::ui
