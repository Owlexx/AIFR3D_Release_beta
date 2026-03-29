#include "dawai/audio_engine/plugin_host/plugin_rack.hpp"

namespace dawai::audio_engine::plugin_host
{

bool PluginRack::setSlot(std::size_t index, std::unique_ptr<PluginInstance> instance,
                         const juce::PluginDescription& description)
{
    if (index >= m_slots.size())
    {
        return false;
    }

    Slot slot;
    slot.instance = std::move(instance);
    slot.description = description;
    m_slots[index] = std::move(slot);
    return true;
}

void PluginRack::setBypass(std::size_t index, bool bypass)
{
    if (index >= m_slots.size() || !m_slots[index].has_value() || !m_slots[index]->instance)
    {
        return;
    }

    m_slots[index]->instance->setBypassed(bypass);
}

void PluginRack::process(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    for (auto& slot : m_slots)
    {
        if (!slot.has_value() || !slot->instance)
        {
            continue;
        }

        slot->instance->process(audio, midi);
    }
}

std::vector<PluginSlotState> PluginRack::saveState() const
{
    std::vector<PluginSlotState> state;
    state.reserve(m_slots.size());

    for (const auto& slot : m_slots)
    {
        if (!slot.has_value() || !slot->instance)
        {
            state.push_back({});
            continue;
        }

        PluginSlotState s;
        s.pluginId = slot->description.fileOrIdentifier.toStdString();
        s.pluginName = slot->description.name.toStdString();
        s.bypass = slot->instance->bypassed();
        s.stateBlob = slot->instance->saveState();
        state.push_back(std::move(s));
    }

    return state;
}

} // namespace dawai::audio_engine::plugin_host
