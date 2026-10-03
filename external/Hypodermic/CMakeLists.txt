cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0025 NEW)
project(Hypodermic VERSION 0.0.1)
set(CMAKE_MACOSX_RPATH OFF)

add_library(Hypodermic INTERFACE)
target_sources(Hypodermic INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>")
target_compile_features(Hypodermic INTERFACE cxx_std_11)
# SYSTEM: this is vendored third-party code. Without it the headers are treated
# as project headers, so -Werror in the consuming target fires on upstream style
# issues (e.g. the old-style cast in Hypodermic/LogLevel.h).
target_include_directories(Hypodermic SYSTEM INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>")
add_library(kruempelmann::Hypodermic ALIAS Hypodermic)
INSTALL(TARGETS Hypodermic
        EXPORT Hypodermic_targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES DESTINATION include)
INSTALL(EXPORT Hypodermic_targets
        DESTINATION lib/cmake/Hypodermic
        FILE Hypodermic_targets.cmake
        NAMESPACE kruempelmann::)
INSTALL(DIRECTORY include/Hypodermic
        DESTINATION include)

# library package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(Hypodermic_config_version.cmake
        COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Hypodermic_config_version.cmake ${CMAKE_CURRENT_BINARY_DIR}/Hypodermic_config_version.cmake
        DESTINATION lib/cmake/Hypodermic)