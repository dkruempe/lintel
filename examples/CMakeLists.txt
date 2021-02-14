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
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(magic_enum REQUIRED)
find_package(spdlog REQUIRED)

# logger example
add_executable(logger_example logger_example.cpp)
target_compile_features(logger_example PUBLIC cxx_std_17)
target_include_directories(logger_example PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(logger_example PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# scheduler example
add_executable(scheduler_example scheduler_example.cpp)
target_compile_features(scheduler_example PUBLIC cxx_std_17)
target_include_directories(scheduler_example PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(scheduler_example PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# file example
add_executable(file_example file_example.cpp)
target_compile_features(file_example PUBLIC cxx_std_17)
target_include_directories(file_example PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(file_example PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# class name example
add_executable(classNameExample classNameExample.cpp)
target_compile_features(classNameExample PUBLIC cxx_std_17)
target_include_directories(classNameExample PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(classNameExample PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# executor example
add_executable(executor_example executor_example.cpp)
target_compile_features(executor_example PUBLIC cxx_std_17)
target_include_directories(executor_example PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(executor_example PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# configuration example
add_executable(configuration_example configuration_example.cpp)
target_compile_features(configuration_example PUBLIC cxx_std_17)
target_include_directories(configuration_example PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(configuration_example PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# spdlog example
add_executable(spdlogExample spdlogExample.cpp)
target_compile_features(spdlogExample PUBLIC cxx_std_17)
target_link_libraries(spdlogExample PUBLIC fmt::fmt spdlog::spdlog)

# stringify extension example
add_executable(stringifyExtensionExample stringifyExtensionExample.cpp)
target_compile_features(stringifyExtensionExample PUBLIC cxx_std_17)
target_include_directories(stringifyExtensionExample PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(stringifyExtensionExample PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# stop watch example
add_executable(stopWatchExample stopWatchExample.cpp)
target_compile_features(stopWatchExample PUBLIC cxx_std_17)
target_include_directories(stopWatchExample PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(stopWatchExample PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# signal example
add_executable(signalExample signalExample.cpp)
target_compile_features(signalExample PUBLIC cxx_std_17)
target_include_directories(signalExample PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(signalExample PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)

# database example
add_executable(databaseExample databaseExample.cpp)
target_compile_features(databaseExample PUBLIC cxx_std_17)
target_include_directories(databaseExample PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(databaseExample PRIVATE kruempelmann::base_library ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite Threads::Threads)
