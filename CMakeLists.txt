cmake_minimum_required(VERSION 3.21)

project(
    rolldown
    VERSION 0.1.0
    DESCRIPTION "Header-only drop tables for games."
    LANGUAGES CXX
)

add_library(rolldown INTERFACE)
add_library(rolldown::rolldown ALIAS rolldown)

target_include_directories(rolldown INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(rolldown INTERFACE cxx_std_17)

option(ROLLDOWN_BUILD_TESTS "Build the rolldown tests and benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(ROLLDOWN_BUILD_EXAMPLES "Build the rolldown examples" ${PROJECT_IS_TOP_LEVEL})
option(ROLLDOWN_WARNINGS_AS_ERRORS "Treat warnings as errors in our own tests, benchmarks, and examples" OFF)
option(ROLLDOWN_SANITIZE "Build our own tests and examples with ASan and UBSan (GCC/Clang only)" OFF)

# Build settings for our own programs only. Never linked to the rolldown library target.
add_library(rolldown_options INTERFACE)
target_compile_options(rolldown_options INTERFACE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion>
    $<$<CXX_COMPILER_ID:GNU>:-Wshadow>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wshadow-all>
)

if(ROLLDOWN_WARNINGS_AS_ERRORS)
    target_compile_options(rolldown_options INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
    )
endif()
if(ROLLDOWN_SANITIZE)
    target_compile_options(rolldown_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_link_options(rolldown_options INTERFACE -fsanitize=address,undefined)
endif()

if(ROLLDOWN_BUILD_TESTS)
    # Download Catch2 (test + benchmark framework) at configure time
    include(FetchContent)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    # Shared settings for our own test/bench code: the library, our build options, Catch2, helpers, xoshiro
    add_library(rolldown_dev INTERFACE)
    target_link_libraries(rolldown_dev INTERFACE rolldown::rolldown rolldown_options Catch2::Catch2WithMain)
    target_include_directories(rolldown_dev INTERFACE tests/support)
    target_include_directories(rolldown_dev SYSTEM INTERFACE third_party)
    target_compile_definitions(rolldown_dev INTERFACE ROLLDOWN_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")

    add_executable(rolldown_tests tests/test_build.cpp tests/test_distribution.cpp tests/test_json_loader.cpp)
    target_link_libraries(rolldown_tests PRIVATE rolldown_dev)
    target_include_directories(rolldown_tests PRIVATE examples/json_loader)

    add_executable(rolldown_bench bench/bench.cpp)
    target_link_libraries(rolldown_bench PRIVATE rolldown_dev)

    # Register every TEST_CASE with CTest so `ctest` can run them
    enable_testing()
    include(Catch)
    catch_discover_tests(rolldown_tests)
endif()

if(ROLLDOWN_BUILD_EXAMPLES)
    add_executable(rolldown_example_json examples/json_loader/main.cpp)
    target_link_libraries(rolldown_example_json PRIVATE rolldown::rolldown rolldown_options)
    target_include_directories(rolldown_example_json SYSTEM PRIVATE third_party) # nlohmann/json
    target_compile_definitions(rolldown_example_json PRIVATE ROLLDOWN_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples/json_loader")
endif()
