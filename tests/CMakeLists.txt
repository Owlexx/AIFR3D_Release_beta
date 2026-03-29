add_executable(dawai_tests
    transport_state_test.cpp
    solo_logic_test.cpp
    session_serialization_test.cpp
    metering_signal_test.cpp
    profile_cache_test.cpp
    ab_switch_test.cpp
    scoring_json_golden_test.cpp
    advisory_layer_test.cpp
    apply_fix_undo_test.cpp
    report_generation_test.cpp
    benchmark_pool_test.cpp
    genre_detector_test.cpp
    plugin_host_smoke_test.cpp)

target_link_libraries(dawai_tests
    PRIVATE
        Catch2::Catch2WithMain
        dawai_audio_engine
        dawai_reference_engine
        dawai_aifr3d_core
        dawai_advisory_layer)

target_compile_features(dawai_tests PRIVATE cxx_std_20)
target_compile_definitions(dawai_tests PRIVATE DAWAI_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_test(NAME dawai_tests COMMAND dawai_tests)

juce_add_console_app(aifred_spectral_pipeline_smoke)
juce_generate_juce_header(aifred_spectral_pipeline_smoke)

target_sources(aifred_spectral_pipeline_smoke
    PRIVATE
        plugin_spectral_pipeline_smoke.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/FeatureExtractor.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/SpectrumAnalyzerFFT.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ScoringEngine.cpp)

target_include_directories(aifred_spectral_pipeline_smoke
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source)

target_link_libraries(aifred_spectral_pipeline_smoke
    PRIVATE
        juce::juce_dsp
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_compile_features(aifred_spectral_pipeline_smoke PRIVATE cxx_std_20)
target_compile_definitions(aifred_spectral_pipeline_smoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)

add_test(NAME aifred_spectral_pipeline_smoke COMMAND aifred_spectral_pipeline_smoke)

juce_add_console_app(aifred_canonical_pipeline_smoke)
juce_generate_juce_header(aifred_canonical_pipeline_smoke)

target_sources(aifred_canonical_pipeline_smoke
    PRIVATE
        aifred_canonical_pipeline_smoke.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/AnalysisEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/DynamicsTracker.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/FeatureExtractor.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/LoudnessEBUR128.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ReferenceModel.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/ScoringEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/SnapshotManager.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/SpectrumAnalyzerFFT.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/StereoAnalyzer.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/TransientDetector.cpp
        ${CMAKE_SOURCE_DIR}/Source/Interpretation/DiagnosticEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/UI/UiModel.cpp)

target_include_directories(aifred_canonical_pipeline_smoke
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/modules/aifr3d_core/include)

target_link_libraries(aifred_canonical_pipeline_smoke
    PRIVATE
        dawai_aifr3d_core
        nlohmann_json::nlohmann_json
        juce::juce_dsp
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_compile_features(aifred_canonical_pipeline_smoke PRIVATE cxx_std_20)
target_compile_definitions(aifred_canonical_pipeline_smoke
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        DAWAI_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_test(NAME aifred_canonical_pipeline_smoke COMMAND aifred_canonical_pipeline_smoke)

add_executable(aifr3d_fix_list_reactivity_smoke
    fix_list_reactivity_smoke.cpp)

target_link_libraries(aifr3d_fix_list_reactivity_smoke
    PRIVATE
        dawai_reference_engine
        dawai_aifr3d_core)

target_compile_features(aifr3d_fix_list_reactivity_smoke PRIVATE cxx_std_20)

add_test(NAME aifr3d_fix_list_reactivity_smoke COMMAND aifr3d_fix_list_reactivity_smoke)
