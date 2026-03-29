#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dawai::ui
{

struct MixCandle
{
    float open = 0.0F;
    float close = 0.0F;
    float high = 0.0F;
    float low = 0.0F;
    std::uint32_t flags = 0;
};

class CandlestickPanel : public juce::Component
{
  public:
    void setCandles(std::vector<MixCandle> candles, std::size_t writeIndex, float baseline,
                    float lastDeltaPct);
    void setAnimationPhase(float phase, float pulse);

    void paint(juce::Graphics& g) override;

  private:
    std::vector<MixCandle> m_candles;
    std::size_t m_writeIndex = 0;
    float m_baseline = 0.5F;
    float m_lastDeltaPct = 0.0F;
    float m_animationPhase = 0.0F;
    float m_animationPulse = 0.0F;
};

} // namespace dawai::ui
