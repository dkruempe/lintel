cmake_minimum_required(VERSION 3.28)
project(cpp-base-library)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16)
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
