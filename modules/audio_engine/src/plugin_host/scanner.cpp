#include "dawai/audio_engine/plugin_host/scanner.hpp"

namespace dawai::audio_engine::plugin_host
{

Scanner::Scanner()
{
    m_formatManager.addDefaultFormats();
}

std::vector<juce::PluginDescription>
Scanner::scanVST3(const std::vector<std::filesystem::path>& searchPaths)
{
    std::vector<juce::PluginDescription> found;

    for (auto* format : m_formatManager.getFormats())
    {
        if (format->getName() != "VST3")
        {
            continue;
        }

        for (const auto& path : searchPaths)
        {
            const juce::FileSearchPath searchPath(path.string());
            const auto files = format->searchPathsForPlugins(searchPath, true, false);
            for (const auto& file : files)
            {
                juce::OwnedArray<juce::PluginDescription> types;
                format->findAllTypesForFile(types, file);

                for (auto* desc : types)
                {
                    m_knownPlugins.addType(*desc);
                    found.push_back(*desc);
                }
            }
        }
    }

    return found;
}

} // namespace dawai::audio_engine::plugin_host
