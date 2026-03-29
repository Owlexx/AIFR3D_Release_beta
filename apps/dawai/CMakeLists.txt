set(DAWAI_BRAND_ICON "${CMAKE_SOURCE_DIR}/assets/icons/aifred_logo.png")

juce_add_gui_app(dawai
    PRODUCT_NAME "AIFR3D Beta 2.2.4"
    COMPANY_NAME "North3rnLight3r"
    ICON_BIG "${DAWAI_BRAND_ICON}"
    ICON_SMALL "${DAWAI_BRAND_ICON}")

target_sources(dawai
    PRIVATE
        src/main.cpp
        src/daemon_server.cpp)

target_link_libraries(dawai
    PRIVATE
        dawai_ui
        dawai_reference_engine
        nlohmann_json::nlohmann_json
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_compile_definitions(dawai
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)
