cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0025 NEW)
project(base_library VERSION 0.0.1)
set(CMAKE_MACOSX_RPATH OFF)

# configuration of default path
set(CONFIG_DIRECTORY "${PROJECT_SOURCE_DIR}/../cfg")
set(LOG_DIRECTORY "${PROJECT_SOURCE_DIR}/../log")

configure_file(
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h.in
        ${PROJECT_SOURCE_DIR}/include/base_library/config.h)

# Download automatically, you can also just copy the conan.cmake file
if (NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake")
endif ()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(REQUIRES fmt/7.0.3 tinyxml2/7.1.0
        OPTIONS
        ${CONAN_EXTRA_OPTIONS}
        BASIC_SETUP
        CMAKE_TARGETS # individual targets to link to
        BUILD
        missing)

# dependencies
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)

add_library(base_library SHARED services/LoggerService.cpp include/base_library/services/LoggerService.h services/SchedulerService.cpp include/base_library/services/SchedulerService.h
        include/base_library/services/FileService.h services/FileService.cpp include/base_library/services/DirectoryService.h services/DirectoryService.cpp include/base_library/models/Property.h
        include/base_library/models/PropertyBase.h include/base_library/services/PropertyService.h services/PropertyService.cpp
        include/base_library/services/StringifyService.h include/base_library/utils/TypeName.h
        include/base_library/exceptions/PropertyNotFoundException.h models/Property.cpp include/base_library/factories/PropertyFactory.h
        factories/PropertyFactory.cpp include/base_library/services/AbstractService.h include/base_library/repositories/PropertyRepository.h
        include/base_library/repositories/NoPropertyRepository.h repositories/NoPropertyRepository.cpp
        include/base_library/repositories/FilePropertyRepository.h repositories/FilePropertyRepository.cpp
        include/base_library/services/ExecutorService.h services/ExecutorService.cpp
        include/base_library/strategies/ConfigSerializationStrategy.h include/base_library/strategies/XMLConfigSerializationStrategy.h
        strategies/XMLConfigSerializationStrategy.cpp include/base_library/utils/JsonSocketAppender.h utils/JsonSocketAppender.cpp
        include/base_library/utils/JsonLayout.h utils/JsonLayout.cpp include/base_library/exceptions/FileServiceIsNotFileException.h
        include/base_library/exceptions/FileServiceFileExists.h include/base_library/exceptions/LoggerServiceNotInitialized.h
        include/base_library/services/StopWatchService.h services/StopWatchService.cpp include/base_library/services/SignalService.h)
target_compile_features(base_library PUBLIC cxx_std_17)
target_include_directories(base_library PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
        PRIVATE
        "$<BUILD_INTERFACE:${CONAN_INCLUDE_DIRS};${LOG4CXX_INCLUDES}>"
        "$<INSTALL_INTERFACE:${CONAN_INCLUDE_DIRS};${LOG4CXX_INCLUDES}>"
        )
target_link_libraries(base_library PRIVATE ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 pthread)
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