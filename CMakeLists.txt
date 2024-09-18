cmake_minimum_required(VERSION 3.16)
project(cpp-base-library)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)
add_library(myproject_options INTERFACE)

# standard compiler warnings
option(myproject_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

#
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
