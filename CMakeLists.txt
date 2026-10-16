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

option(ROLLDOWN_BUILD_BENCH "Build the rolldown benchmark" ${PROJECT_IS_TOP_LEVEL})

if(ROLLDOWN_BUILD_BENCH)
    add_executable(rolldown_bench bench/bench.cpp)
    target_link_libraries(rolldown_bench PRIVATE rolldown::rolldown)
    # Turning on compiler warnings
    target_compile_options(rolldown_bench PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wconversion>
    )
endif()
