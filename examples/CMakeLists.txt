cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

#dependencies
if (NOT TARGET kruempelmann::base_library)
    find_package(base_library REQUIRED)
    include_directories(${base_library_INCLUDE_DIRS})
endif ()

# main example
add_executable(main main.cpp)
target_compile_features(main PUBLIC cxx_std_17)
target_include_directories(main PRIVATE ${magic_enum_INCLUDE_DIRS})
target_link_libraries(main PRIVATE kruempelmann::base_library)