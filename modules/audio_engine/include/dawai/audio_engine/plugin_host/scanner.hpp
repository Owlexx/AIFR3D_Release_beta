#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <filesystem>
#include <vector>

namespace dawai::audio_engine::plugin_host
{

class Scanner
{
  public:
    Scanner();

    [[nodiscard]] std::vector<juce::PluginDescription>
    scanVST3(const std::vector<std::filesystem::path>& searchPaths);

    [[nodiscard]] juce::AudioPluginFormatManager& formatManager() noexcept
    {
        return m_formatManager;
    }
    [[nodiscard]] const juce::AudioPluginFormatManager& formatManager() const noexcept
    {
        return m_formatManager;
    }

  private:
    juce::AudioPluginFormatManager m_formatManager;
    juce::KnownPluginList m_knownPlugins;
};

} // namespace dawai::audio_engine::plugin_host
