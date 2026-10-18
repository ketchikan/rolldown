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

if(ROLLDOWN_BUILD_TESTS)
    # Download Catch2 (test + benchmark framework) at configure time
    include(FetchContent)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    # Shared settings for our own test/bench code: the library, Catch2, helpers, xoshiro, warnings
    add_library(rolldown_dev INTERFACE)
    target_link_libraries(rolldown_dev INTERFACE rolldown::rolldown Catch2::Catch2WithMain)
    target_include_directories(rolldown_dev INTERFACE tests/support third_party)
    target_compile_definitions(rolldown_dev INTERFACE ROLLDOWN_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    # Turning on compiler warnings
    target_compile_options(rolldown_dev INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion>
    )

    add_executable(rolldown_tests tests/test_build.cpp tests/test_distribution.cpp tests/test_json_loader.cpp)
    target_link_libraries(rolldown_tests PRIVATE rolldown_dev)

    add_executable(rolldown_bench bench/bench.cpp)
    target_link_libraries(rolldown_bench PRIVATE rolldown_dev)

    # Register every TEST_CASE with CTest so `ctest` can run them
    enable_testing()
    include(Catch)
    catch_discover_tests(rolldown_tests)
endif()
