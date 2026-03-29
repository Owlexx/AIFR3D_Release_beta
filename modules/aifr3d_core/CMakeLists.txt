add_library(dawai_aifr3d_core
    src/feature_ingestion.cpp
    src/scoring.cpp
    src/fix_list.cpp
    src/analysis_writer.cpp
    src/apply_fix.cpp
    src/session_store.cpp
    src/reporting/mix_reporter.cpp)

set_target_properties(dawai_aifr3d_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(dawai_aifr3d_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(dawai_aifr3d_core
    PUBLIC
        dawai_reference_engine
        nlohmann_json::nlohmann_json)

target_compile_features(dawai_aifr3d_core PUBLIC cxx_std_20)
