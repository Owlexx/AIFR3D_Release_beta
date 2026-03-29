cmake_minimum_required(VERSION 3.24)
project(DawAI VERSION 2.2.4 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(JUCE_MODAL_LOOPS_PERMITTED=1)
add_compile_definitions(DAWAI_CANONICAL_VERSION="${PROJECT_VERSION}")

option(DAWAI_BUILD_APP "Build the DAW executable" ON)
option(DAWAI_BUILD_AIFRED_PLUGIN "Build the AIFRED VST3 plugin target" ON)
option(DAWAI_ENABLE_TESTS "Build unit tests" ON)
option(DAWAI_ENABLE_CLANG_TIDY "Enable clang-tidy for targets" OFF)

include(cmake/CPM.cmake)

CPMAddPackage("gh:nlohmann/json@3.11.3")
CPMAddPackage("gh:catchorg/Catch2@3.7.1")
CPMAddPackage(
    NAME juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6)

if(DAWAI_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy)
    endif()
endif()

add_subdirectory(modules/reference_engine)
add_subdirectory(modules/aifr3d_core)
add_subdirectory(modules/advisory_layer)
add_subdirectory(modules/audio_engine)
add_subdirectory(modules/ui)

if(DAWAI_BUILD_APP)
    add_subdirectory(apps/dawai)
endif()

if(DAWAI_BUILD_AIFRED_PLUGIN)
    add_subdirectory(Source)
endif()

if(DAWAI_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(DIRECTORY assets DESTINATION .)
install(DIRECTORY docs DESTINATION .)
install(DIRECTORY scripts DESTINATION . OPTIONAL)
