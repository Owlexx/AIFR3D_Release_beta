add_library(dawai_ui
    src/theme/look_and_feel.cpp
    src/transport_bar.cpp
    src/mixer_strip.cpp
    src/timeline_component.cpp
    src/meter_panel.cpp
    src/insight_panel.cpp
    src/candlestick_panel.cpp
    src/aifr3d_panel.cpp
    src/main_view.cpp)

target_include_directories(dawai_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(dawai_ui
    PUBLIC
        dawai_audio_engine
        dawai_aifr3d_core
        dawai_advisory_layer
        dawai_reference_engine
        juce::juce_gui_basics
        juce::juce_gui_extra)

target_compile_features(dawai_ui PUBLIC cxx_std_20)
