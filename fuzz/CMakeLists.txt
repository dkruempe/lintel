cmake_minimum_required(VERSION 3.16)

# Fuzzing harnesses for the library's parsers.
#
# With Clang each harness is a real libFuzzer target (-fsanitize=fuzzer,address).
# With any other compiler (e.g. GCC, which has no libFuzzer) the same
# LLVMFuzzerTestOneInput is wrapped by StandaloneFuzzDriver.cpp so the harness
# still compiles and can be smoke-tested on a corpus:
#
#   cmake -S . -B build/fuzz -DBUILD_FUZZERS=ON -DCMAKE_BUILD_TYPE=Debug ...
#   cmake --build build/fuzz --target fuzz_json_dto
#   ./bin/fuzz_json_dto fuzz/corpus/json_dto/*
set(LINTEL_FUZZ_TARGETS
    fuzz_config_env
    fuzz_json_dto
    fuzz_property_xml
)

foreach(target IN LISTS LINTEL_FUZZ_TARGETS)
    add_executable(${target} ${target}.cpp)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_link_libraries(${target} PRIVATE
        lintel::lintel
        project_warnings
        lintel_options
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target} PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer -g)
        target_link_options(${target} PRIVATE -fsanitize=fuzzer,address)
    else()
        target_sources(${target} PRIVATE StandaloneFuzzDriver.cpp)
        message(STATUS "fuzz: ${target} uses the standalone driver (no libFuzzer); use Clang for real fuzzing")
    endif()
endforeach()
