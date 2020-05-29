cmake_minimum_required(VERSION 3.17)
project(Logging)

# c++ standard
set(CMAKE_CXX_STANDARD 17)

# dependencies
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)
find_library(FMT NAMES fmt)
find_path(FMT_INCLUDES NAMES fmt/format.h)

include_directories(include ${LOG4CXX_INCLUDES} ${FMT_INCLUDES})
add_library(base src/Logger.cpp include/Logger.h src/Scheduler.cpp include/Scheduler.h
        include/File.h src/File.cpp include/Directory.h src/Directory.cpp)
target_link_libraries(base ${LOG4CXX} ${FMT})

# Examples
ADD_SUBDIRECTORY(examples)

# configuration of default path
set(CONFIG_DIRECTORY "${PROJECT_SOURCE_DIR}/cfg")
set(BINARY_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(LOG_DIRECTORY "${PROJECT_SOURCE_DIR}/log")

configure_file(
        ${PROJECT_SOURCE_DIR}/include/config.h.in
        ${PROJECT_SOURCE_DIR}/include/config.h
)