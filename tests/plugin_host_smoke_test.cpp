#include "dawai/audio_engine/plugin_host/plugin_instance.hpp"
#include "dawai/audio_engine/plugin_host/scanner.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

TEST_CASE("Plugin host can load known plugin and process silence", "[plugin][smoke]")
{
    const char* pluginPath = std::getenv("DAWAI_TEST_VST3");
    if (pluginPath == nullptr)
    {
        SUCCEED("Skipping: DAWAI_TEST_VST3 is not set");
        return;
    }

    dawai::audio_engine::plugin_host::Scanner scanner;
    const auto plugins = scanner.scanVST3({pluginPath});
    if (plugins.empty())
    {
        SUCCEED("Skipping: no plugin found at DAWAI_TEST_VST3");
        return;
    }

    juce::String error;
    auto instance = dawai::audio_engine::plugin_host::PluginInstance::create(
        plugins.front(), scanner.formatManager(), 48000.0, 512, error);

    REQUIRE(instance != nullptr);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;

    instance->process(buffer, midi);
    SUCCEED();
}
