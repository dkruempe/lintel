cmake_minimum_required(VERSION 3.17)
project(Logging)

# c++ standard
set(CMAKE_CXX_STANDARD 17)

# dependencies
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)
find_library(FMT NAMES fmt)
find_path(FMT_INCLUDES NAMES fmt/format.h)

# options
option(enable_examples "Enables/Disabled examples" ON)

include_directories(include ${LOG4CXX_INCLUDES} ${FMT_INCLUDES})
add_library(base Logger.cpp Logger.h Scheduler.cpp Scheduler.h)
target_link_libraries(base ${LOG4CXX} ${FMT})

# Examples
#if (enable_examples)
    add_executable(logger_example logger_example.cpp)
    target_link_libraries(logger_example base)
    add_executable(scheduler_example scheduler_example.cpp)
    target_link_libraries(scheduler_example base)
#endif ()

set(CONFIG_DIRECTORY "${PROJECT_SOURCE_DIR}/cfg")
set(BINARY_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(LOG_DIRECTORY "${PROJECT_SOURCE_DIR}/log")

# configure header file
configure_file(
        ${PROJECT_SOURCE_DIR}/config.h.in
        ${PROJECT_SOURCE_DIR}/config.h
)