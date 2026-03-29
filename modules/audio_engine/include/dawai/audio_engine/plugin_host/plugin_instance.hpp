#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace dawai::audio_engine::plugin_host
{

class PluginInstance
{
  public:
    static std::unique_ptr<PluginInstance> create(const juce::PluginDescription& description,
                                                  juce::AudioPluginFormatManager& formatManager,
                                                  double sampleRate, int blockSize,
                                                  juce::String& error);

    explicit PluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance);

    void setBypassed(bool bypassed) noexcept
    {
        m_bypassed = bypassed;
    }
    [[nodiscard]] bool bypassed() const noexcept
    {
        return m_bypassed;
    }

    void process(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi);

    [[nodiscard]] std::vector<std::uint8_t> saveState() const;
    void restoreState(const std::vector<std::uint8_t>& state);

    [[nodiscard]] juce::AudioPluginInstance* raw() noexcept
    {
        return m_instance.get();
    }

  private:
    std::unique_ptr<juce::AudioPluginInstance> m_instance;
    bool m_bypassed = false;
};

} // namespace dawai::audio_engine::plugin_host
