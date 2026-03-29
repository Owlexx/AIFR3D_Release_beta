add_library(dawai_audio_engine
    src/transport_controller.cpp
    src/session_serializer.cpp
    src/audio_device_service.cpp
    src/playback_engine.cpp
    src/module_profiler.cpp
    src/plugin_host/scanner.cpp
    src/plugin_host/plugin_instance.cpp
    src/plugin_host/plugin_rack.cpp)

target_include_directories(dawai_audio_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(dawai_audio_engine
    PUBLIC
        dawai_reference_engine
        nlohmann_json::nlohmann_json
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_events
        juce::juce_core)

target_compile_definitions(dawai_audio_engine
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_compile_features(dawai_audio_engine PUBLIC cxx_std_20)
