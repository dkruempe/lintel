cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

#dependencies
if (NOT TARGET kruempelmann::base_library)
    find_package(base_library REQUIRED)
    include_directories(${base_library_INCLUDE_DIRS})
endif ()

# dependencies
if (NOT TARGET kruempelmann::Hypodermic)
    find_package(Hypodermic REQUIRED)
    include_directories(${Hypodermic_INCLUDE_DIRS})
else ()
    include_directories(../external/Hypodermic/include)
endif ()

# dependencies
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(magic_enum REQUIRED)
find_package(spdlog REQUIRED)
find_package(httplib REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost 1.75.0 REQUIRED COMPONENTS container) # interprocess process asio are header only libraries

# spdlog example
add_executable(spdlogExample spdlogExample.cpp)
target_compile_features(spdlogExample PUBLIC cxx_std_17)
target_link_libraries(spdlogExample PUBLIC fmt::fmt spdlog::spdlog)

# main example
add_executable(main main.cpp)
target_compile_features(main PUBLIC cxx_std_17)
target_include_directories(main PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(main PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads OpenSSL::SSL OpenSSL::Crypto httplib::httplib)