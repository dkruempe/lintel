cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0025 NEW)
project(base_library VERSION 0.0.1)
set(CMAKE_MACOSX_RPATH OFF)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

# configuration of default path
if (NOT DEFINED CONFIG_DIRECTORY)
    set(CONFIG_DIRECTORY "${PROJECT_SOURCE_DIR}/../cfg")
endif ()

if (NOT DEFINED LOG_DIRECTORY)
    set(LOG_DIRECTORY "${PROJECT_SOURCE_DIR}/../log")
endif ()

if (NOT DEFINED BOOTSTRAP_CONFIG_NAME)
    set(BOOTSTRAP_CONFIG_NAME "bootstrap")
endif ()

configure_file(
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h.in
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h)

# dependencies
if (NOT TARGET kruempelmann::Hypodermic)
    find_package(Hypodermic REQUIRED)
    include_directories(${Hypodermic_INCLUDE_DIRS})
else ()
    include_directories(../external/Hypodermic/include)
endif ()
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(magic_enum REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost 1.75.0 REQUIRED)
find_package(RapidJSON REQUIRED)

# build
FILE(GLOB_RECURSE headers include/*.h)
FILE(GLOB_RECURSE sources *.cpp)

add_library(base_library SHARED ${headers} ${sources})
target_compile_features(base_library PUBLIC cxx_std_17)
target_include_directories(base_library
        SYSTEM PUBLIC
        "$<BUILD_INTERFACE:${OpenSSL_INCLUDES};${magic_enum_INCLUDE_DIRS};${LOG4CXX_INCLUDES};${Boost_INCLUDE_DIRS};${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${OpenSSL_INCLUDES};${magic_enum_INCLUDE_DIRS};${LOG4CXX_INCLUDES};${Boost_INCLUDE_DIRS};include>"
        )
target_link_libraries(base_library PRIVATE ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite3 Threads::Threads OpenSSL::SSL OpenSSL::Crypto RapidJSON::RapidJSON project_warnings)
add_library(kruempelmann::base_library ALIAS base_library)
INSTALL(TARGETS base_library
        EXPORT base_library_targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
        INCLUDES DESTINATION include)
INSTALL(EXPORT base_library_targets
        DESTINATION lib/cmake/base_library
        FILE base_library_targets.cmake
        NAMESPACE kruempelmann::)
INSTALL(DIRECTORY include/base_library
        DESTINATION include)

# library package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(base_library_config_version.cmake
        COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/base_library_config_version.cmake ${CMAKE_CURRENT_BINARY_DIR}/base_library_config_version.cmake
        DESTINATION lib/cmake/base_library)