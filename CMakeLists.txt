cmake_minimum_required(VERSION 3.17)
project(Logging)

set(CMAKE_CXX_STANDARD 17)

find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)
find_library(FMT NAMES fmt)
find_path(FMT_INCLUDES NAMES fmt/format.h)

include_directories(include ${LOG4CXX_INCLUDES} ${FMT_INCLUDES})
add_library(logging Logger.cpp Logger.h Scheduler.cpp Scheduler.h)
target_link_libraries(logging ${LOG4CXX} ${FMT})

add_executable(logger_example logger_example.cpp)
target_link_libraries(logger_example logging)

add_executable(scheduler_example scheduler_example.cpp)
target_link_libraries(scheduler_example logging)

set(CONFIG_DIRECTORY "${PROJECT_SOURCE_DIR}/cfg")
set(BINARY_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(LOG_DIRECTORY "${PROJECT_SOURCE_DIR}/log")

# configure header file
configure_file(
        ${PROJECT_SOURCE_DIR}/config.h.in
        ${PROJECT_SOURCE_DIR}/config.h
)