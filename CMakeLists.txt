cmake_minimum_required(VERSION 3.16)
project(cpp-base-library)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# standard compiler warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

# Library
ADD_SUBDIRECTORY(external)
ADD_SUBDIRECTORY(src)

# Examples
ADD_SUBDIRECTORY(examples)

# Tests
enable_testing()
ADD_SUBDIRECTORY(tests)