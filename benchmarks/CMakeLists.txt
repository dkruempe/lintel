cmake_minimum_required(VERSION 3.16)
project(base_benchmarks)
cmake_policy(SET CMP0025 NEW)

find_package(benchmark REQUIRED)

add_executable(base_benchmarks
    RegexBenchmark.cpp
    SqliteBenchmark.cpp
    PropertyDtoBenchmark.cpp
)
target_compile_features(base_benchmarks PUBLIC cxx_std_17)
target_link_libraries(base_benchmarks PRIVATE
    lintel::lintel
    benchmark::benchmark_main
    project_warnings
    lintel_options
)

add_custom_command(TARGET base_benchmarks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:base_benchmarks> ${CMAKE_SOURCE_DIR}/bin/base_benchmarks
)
