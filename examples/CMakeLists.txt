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

conan_cmake_run(REQUIRES fmt/6.2.0 spdlog/1.6.1
        OPTIONS
        ${CONAN_EXTRA_OPTIONS}
        BASIC_SETUP
        CMAKE_TARGETS # individual targets to link to
        BUILD
        missing)

add_executable(logger_example logger_example.cpp)
target_compile_features(logger_example PUBLIC cxx_std_17)
target_link_libraries(logger_example PUBLIC kruempelmann::base_library)
add_executable(scheduler_example scheduler_example.cpp)
target_compile_features(scheduler_example PUBLIC cxx_std_17)
target_link_libraries(scheduler_example PUBLIC kruempelmann::base_library)
add_executable(file_example file_example.cpp)
target_compile_features(file_example PUBLIC cxx_std_17)
target_link_libraries(file_example PUBLIC kruempelmann::base_library)
add_executable(classNameExample classNameExample.cpp)
target_compile_features(classNameExample PUBLIC cxx_std_17)
target_link_libraries(classNameExample PUBLIC kruempelmann::base_library)
add_executable(executor_example executor_example.cpp)
target_compile_features(executor_example PUBLIC cxx_std_17)
target_link_libraries(executor_example PUBLIC kruempelmann::base_library)
add_executable(configuration_example configuration_example.cpp)
target_compile_features(configuration_example PUBLIC cxx_std_17)
target_link_libraries(configuration_example PUBLIC kruempelmann::base_library)
if (NOT APPLE)
    add_executable(spdlogExample spdlogExample.cpp)
    target_include_directories(spdlogExample PUBLIC "${CONAN_INCLUDE_DIRS}")
    target_compile_features(spdlogExample PUBLIC cxx_std_17)
    target_link_libraries(spdlogExample PUBLIC CONAN_PKG::fmt CONAN_PKG::spdlog)
endif ()
add_executable(stringifyExtensionExample stringifyExtensionExample.cpp)
target_compile_features(stringifyExtensionExample PUBLIC cxx_std_17)
target_link_libraries(stringifyExtensionExample PUBLIC kruempelmann::base_library)