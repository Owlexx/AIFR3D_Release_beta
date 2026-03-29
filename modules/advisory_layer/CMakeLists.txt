add_library(dawai_advisory_layer
    src/api_key_store.cpp
    src/analysis_sandbox.cpp
    src/openai_adviser.cpp
    src/local_brain_adviser.cpp
    src/memory_store.cpp
    src/advisory_service.cpp)

set_target_properties(dawai_advisory_layer PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(dawai_advisory_layer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(dawai_advisory_layer
    PUBLIC
        dawai_aifr3d_core
        nlohmann_json::nlohmann_json)

target_compile_features(dawai_advisory_layer PUBLIC cxx_std_20)
