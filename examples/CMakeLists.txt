cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)

#dependencies
if (NOT TARGET kruempelmann::base_library)
    find_package(base_library REQUIRED)
    include_directories(${base_library_INCLUDE_DIRS})
endif ()

# Download automatically, you can also just copy the conan.cmake file
if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake")
endif ()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(REQUIRES fmt/7.0.3 spdlog/1.8.0 tinyxml2/7.1.0
        OPTIONS
        ${CONAN_EXTRA_OPTIONS}
        BASIC_SETUP
        CMAKE_TARGETS # individual targets to link to
        BUILD
        missing)

# dependencies
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)

find_package(Threads REQUIRED)

# logger example
add_executable(logger_example logger_example.cpp)
target_compile_features(logger_example PUBLIC cxx_std_17)
target_include_directories(logger_example PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(logger_example PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# scheduler example
add_executable(scheduler_example scheduler_example.cpp)
target_compile_features(scheduler_example PUBLIC cxx_std_17)
target_include_directories(scheduler_example PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(scheduler_example PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# file example
add_executable(file_example file_example.cpp)
target_compile_features(file_example PUBLIC cxx_std_17)
target_include_directories(file_example PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(file_example PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# class name example
add_executable(classNameExample classNameExample.cpp)
target_compile_features(classNameExample PUBLIC cxx_std_17)
target_include_directories(classNameExample PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(classNameExample PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# executor example
add_executable(executor_example executor_example.cpp)
target_compile_features(executor_example PUBLIC cxx_std_17)
target_include_directories(executor_example PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(executor_example PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# configuration example
add_executable(configuration_example configuration_example.cpp)
target_compile_features(configuration_example PUBLIC cxx_std_17)
target_include_directories(configuration_example PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(configuration_example PUBLIC kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# spdlog example
add_executable(spdlogExample spdlogExample.cpp)
target_include_directories(spdlogExample PUBLIC "${CONAN_INCLUDE_DIRS}")
target_compile_features(spdlogExample PUBLIC cxx_std_17)
target_link_libraries(spdlogExample PUBLIC CONAN_PKG::fmt CONAN_PKG::spdlog)

# stringify extension example
add_executable(stringifyExtensionExample stringifyExtensionExample.cpp)
target_compile_features(stringifyExtensionExample PUBLIC cxx_std_17)
target_include_directories(stringifyExtensionExample PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(stringifyExtensionExample PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# stop watch example
add_executable(stopWatchExample stopWatchExample.cpp)
target_compile_features(stopWatchExample PUBLIC cxx_std_17)
target_include_directories(stopWatchExample PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(stopWatchExample PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)

# signal example
add_executable(signalExample signalExample.cpp)
target_compile_features(signalExample PUBLIC cxx_std_17)
target_include_directories(signalExample PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(signalExample PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 Threads::Threads)