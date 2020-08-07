cmake_minimum_required(VERSION 3.16)
project(cpp-basic-libraries)

# Library
ADD_SUBDIRECTORY(src)
# Examples
ADD_SUBDIRECTORY(examples)

# Tests
enable_testing()
ADD_SUBDIRECTORY(tests)