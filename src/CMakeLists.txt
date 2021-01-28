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

configure_file(
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h.in
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h)

# dependencies
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)

find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(magic_enum REQUIRED)
find_package(SQLite3 REQUIRED)

# build
FILE(GLOB_RECURSE headers include/*.h)
FILE(GLOB_RECURSE sources *.cpp)

add_library(base_library SHARED ${headers} ${sources})
target_compile_features(base_library PUBLIC cxx_std_17)
target_include_directories(base_library PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
        PRIVATE
        "$<BUILD_INTERFACE:${magic_enum_INCLUDE_DIRS};${LOG4CXX_INCLUDES}>"
        "$<INSTALL_INTERFACE:${magic_enum_INCLUDE_DIRS};${LOG4CXX_INCLUDES}>"
        )
target_link_libraries(base_library PRIVATE ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL SQLite::SQLite3 Threads::Threads)
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