#include "dawai/audio_engine/plugin_host/plugin_instance.hpp"

namespace dawai::audio_engine::plugin_host
{

std::unique_ptr<PluginInstance>
PluginInstance::create(const juce::PluginDescription& description,
                       juce::AudioPluginFormatManager& formatManager, double sampleRate,
                       int blockSize, juce::String& error)
{
    try
    {
        auto instance =
            formatManager.createPluginInstance(description, sampleRate, blockSize, error);
        if (instance == nullptr)
        {
            return nullptr;
        }

        instance->prepareToPlay(sampleRate, blockSize);
        return std::make_unique<PluginInstance>(std::move(instance));
    }
    catch (...)
    {
        error = "Plugin instantiation failed with exception";
        return nullptr;
    }
}

PluginInstance::PluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance)
    : m_instance(std::move(instance))
{
}

void PluginInstance::process(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    if (m_instance == nullptr || m_bypassed)
    {
        return;
    }

    try
    {
        m_instance->processBlock(audio, midi);
    }
    catch (...)
    {
        m_bypassed = true;
    }
}

std::vector<std::uint8_t> PluginInstance::saveState() const
{
    std::vector<std::uint8_t> blob;
    if (!m_instance)
    {
        return blob;
    }

    juce::MemoryBlock state;
    m_instance->getStateInformation(state);

    const auto* begin = static_cast<const std::uint8_t*>(state.getData());
    blob.assign(begin, begin + state.getSize());
    return blob;
}

void PluginInstance::restoreState(const std::vector<std::uint8_t>& state)
{
    if (!m_instance || state.empty())
    {
        return;
    }

    m_instance->setStateInformation(state.data(), static_cast<int>(state.size()));
}

} // namespace dawai::audio_engine::plugin_host
