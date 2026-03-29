add_library(dawai_reference_engine
    ../metering/src/fft_analyzer.cpp
    ../metering/src/loudness_analyzer.cpp
    ../metering/src/stereo_analyzer.cpp
    ../metering/src/metering_engine.cpp
    src/reference_library.cpp
    src/profile_cache.cpp
    src/reference_profiler.cpp
    src/ab_controller.cpp
    src/pools/benchmark_pool.cpp
    src/genre_detector.cpp)

target_include_directories(dawai_reference_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../metering/include)

target_link_libraries(dawai_reference_engine
    PUBLIC
        nlohmann_json::nlohmann_json)

target_compile_features(dawai_reference_engine PUBLIC cxx_std_20)
