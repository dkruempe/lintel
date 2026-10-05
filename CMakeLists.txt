cmake_minimum_required(VERSION 3.16)
set(BASE_LIBRARY_VERSION "0.1.0" CACHE STRING "Release version of base_library (SemVer)")
project(cpp-base-library VERSION ${BASE_LIBRARY_VERSION})

# Single source of truth for the version number: BASE_LIBRARY_VERSION above. src/include/base_library/config.h
# is generated from it (VERSION_MAJOR/MINOR/PATCH) and release.yml checks the git tag against it, so the version
# in swagger.yaml - which documents the same release - has to follow. Fail at configure time instead of shipping
# two different version numbers.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/swagger.yaml" BASE_LIBRARY_SWAGGER_SPEC)
string(REGEX MATCHALL
    "\n  version:[ \t]*\"?([0-9]+\\.[0-9]+\\.[0-9]+)\"?[ \t\r]*\n"
    BASE_LIBRARY_SWAGGER_VERSION_MATCHES "${BASE_LIBRARY_SWAGGER_SPEC}")
list(LENGTH BASE_LIBRARY_SWAGGER_VERSION_MATCHES BASE_LIBRARY_SWAGGER_VERSION_COUNT)
if(BASE_LIBRARY_SWAGGER_VERSION_COUNT EQUAL 0)
    message(FATAL_ERROR "swagger.yaml contains no 'info.version' entry - cannot verify it against ${BASE_LIBRARY_VERSION}")
endif()
if(NOT BASE_LIBRARY_SWAGGER_VERSION_COUNT EQUAL 1)
    message(FATAL_ERROR "swagger.yaml contains ${BASE_LIBRARY_SWAGGER_VERSION_COUNT} 'info.version' entries - expected exactly one")
endif()
string(REGEX REPLACE "\n  version:[ \t]*\"?([0-9]+\\.[0-9]+\\.[0-9]+)\"?[ \t\r]*\n" "\\1"
    BASE_LIBRARY_SWAGGER_VERSION "${BASE_LIBRARY_SWAGGER_VERSION_MATCHES}")
if(NOT BASE_LIBRARY_SWAGGER_VERSION STREQUAL BASE_LIBRARY_VERSION)
    message(FATAL_ERROR
        "swagger.yaml declares version '${BASE_LIBRARY_SWAGGER_VERSION}', "
        "but BASE_LIBRARY_VERSION is '${BASE_LIBRARY_VERSION}'. "
        "Update swagger.yaml or BASE_LIBRARY_VERSION in this file so both match.")
endif()
message(STATUS "swagger.yaml version matches BASE_LIBRARY_VERSION: ${BASE_LIBRARY_SWAGGER_VERSION}")
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Install-Pfade (lib, include, share) zentral festlegen, damit CMake und alle
# Unterverzeichnisse dieselben Zielordner benutzen. Muss im Top-Level passieren,
# sonst waechst in jedem Unterverzeichnis ein eigener Satz Variablen.
include(GNUInstallDirs)

# Hilfsfunktionen fuer das CMake-Paket: configure_package_config_file() erzeugt aus
# cmake/base_libraryConfig.cmake.in das installierte base_libraryConfig.cmake.
include(CMakePackageConfigHelpers)

# CCache - compiler cache for faster rebuilds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
endif()

# Unity/Jumbo builds - combine small translation units
option(ENABLE_UNITY_BUILD "Enable unity/jumbo builds" ON)
if(ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 32)
endif()

# LTO/IPO - interprocedural optimization (off by default, can increase link time)
option(ENABLE_LTO "Enable Link Time Optimization" OFF)
if(ENABLE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)
add_library(myproject_options INTERFACE)

# standard compiler warnings
option(myproject_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# static analyzers
option(MYPROJECT_ENABLE_CLANG_TIDY "Enable clang-tidy during build (slow)" OFF)
include(cmake/StaticAnalyzers.cmake)
myproject_enable_clang_tidy(myproject_options ${myproject_WARNINGS_AS_ERRORS})
myproject_enable_cppcheck(${myproject_WARNINGS_AS_ERRORS} "" # override cppcheck options
)


# Library
ADD_SUBDIRECTORY(external)
ADD_SUBDIRECTORY(src)

# Examples
ADD_SUBDIRECTORY(examples)

# Tests
enable_testing()
ADD_SUBDIRECTORY(tests)

# Benchmarks (off by default)
option(BUILD_BENCHMARKS "Build benchmark suite" OFF)
if(BUILD_BENCHMARKS)
    ADD_SUBDIRECTORY(benchmarks)
endif()
