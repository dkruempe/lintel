cmake_minimum_required(VERSION 3.16)
project(cpp-base-library)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

# Library
ADD_SUBDIRECTORY(external)
ADD_SUBDIRECTORY(src)

# Examples
ADD_SUBDIRECTORY(examples)

# Tests
enable_testing()
ADD_SUBDIRECTORY(tests)