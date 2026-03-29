set(AIFR3D_BRAND_ICON "${CMAKE_SOURCE_DIR}/assets/icons/aifred_logo.png")

set(AIFRED_PLUGIN_SOURCES
    Plugin/PluginProcessor.h
    Plugin/PluginProcessor.cpp
    Plugin/PluginEditor.h
    Plugin/PluginEditor.cpp
    Plugin/Parameters.h
    Plugin/Parameters.cpp
    Plugin/LookAndFeel_N3L.h
    Plugin/LookAndFeel_N3L.cpp
    Plugin/Theme.h
    Common/AnalysisTypes.h
    Common/MixMetrics.h
    Common/SimpleLockFreeQueue.h
    Interpretation/AifredReport.h
    Interpretation/DiagnosticEngine.h
    Interpretation/DiagnosticEngine.cpp
    DSP/AnalysisEngine.h
    DSP/AnalysisEngine.cpp
    DSP/FeatureExtractor.h
    DSP/FeatureExtractor.cpp
    DSP/LoudnessEBUR128.h
    DSP/LoudnessEBUR128.cpp
    DSP/SpectrumAnalyzerFFT.h
    DSP/SpectrumAnalyzerFFT.cpp
    DSP/DynamicsTracker.h
    DSP/DynamicsTracker.cpp
    DSP/TransientDetector.h
    DSP/TransientDetector.cpp
    DSP/StereoAnalyzer.h
    DSP/StereoAnalyzer.cpp
    DSP/ReferenceModel.h
    DSP/ReferenceModel.cpp
    DSP/ScoringEngine.h
    DSP/ScoringEngine.cpp
    DSP/SnapshotManager.h
    DSP/SnapshotManager.cpp
    DSP/JsonReportWriter.h
    DSP/JsonReportWriter.cpp
    UI/UiModel.h
    UI/UiModel.cpp
    UI/MainView.h
    UI/MainView.cpp
    UI/TopBar.h
    UI/TopBar.cpp
    UI/MixSignatureMeter.h
    UI/MixSignatureMeter.cpp
    UI/ApprovalHalo.h
    UI/ApprovalHalo.cpp
    UI/CandleStoryline.h
    UI/CandleStoryline.cpp
    UI/StereoArcView.h
    UI/StereoArcView.cpp
    UI/FixListPanel.h
    UI/FixListPanel.cpp
    UI/MixTipsPanel.h
    UI/MixTipsPanel.cpp)

function(add_aifred_plugin_variant target suffix variant_num plugin_code)
    juce_add_plugin(${target}
        COMPANY_NAME "North3rnLight3r"
        BUNDLE_ID "com.north3rnlight3r.aifred.${suffix}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        PLUGIN_MANUFACTURER_CODE N3L1
        PLUGIN_CODE ${plugin_code}
        VERSION ${PROJECT_VERSION}
        FORMATS VST3 Standalone
        ICON_BIG "${AIFR3D_BRAND_ICON}"
        ICON_SMALL "${AIFR3D_BRAND_ICON}"
        PRODUCT_NAME "AIFRED_${suffix}")

    juce_generate_juce_header(${target})

    target_sources(${target}
        PRIVATE
            ${AIFRED_PLUGIN_SOURCES})

    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(${target} PRIVATE cxx_std_20)

    target_link_libraries(${target}
        PRIVATE
            dawai_advisory_layer
            nlohmann_json::nlohmann_json
            juce::juce_dsp
            juce::juce_audio_utils
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            AIFRED_UI_VARIANT=${variant_num})
endfunction()

add_aifred_plugin_variant(aifred_vst3_A a 1 AfdA)
add_aifred_plugin_variant(aifred_vst3_B b 2 AfdB)
add_aifred_plugin_variant(aifred_vst3_C c 3 AfdC)
add_aifred_plugin_variant(aifred_vst3_D d 4 AfdD)

# Compatibility aggregate target for scripts that still invoke "aifred_vst3".
add_custom_target(aifred_vst3 DEPENDS aifred_vst3_A)
