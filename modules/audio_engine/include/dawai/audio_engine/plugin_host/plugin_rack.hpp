#pragma once

#include "dawai/audio_engine/plugin_host/plugin_instance.hpp"
#include "dawai/audio_engine/track_model.hpp"

#include <array>
#include <optional>

namespace dawai::audio_engine::plugin_host
{

class PluginRack
{
  public:
    static constexpr std::size_t kSlots = 4;

    bool setSlot(std::size_t index, std::unique_ptr<PluginInstance> instance,
                 const juce::PluginDescription& description);
    void setBypass(std::size_t index, bool bypass);

    void process(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi);

    [[nodiscard]] std::vector<PluginSlotState> saveState() const;

  private:
    struct Slot
    {
        std::unique_ptr<PluginInstance> instance;
        juce::PluginDescription description;
    };

    std::array<std::optional<Slot>, kSlots> m_slots;
};

} // namespace dawai::audio_engine::plugin_host
