cmake_minimum_required(VERSION 3.0)
project(Logging)

# c++ standard
set(CMAKE_CXX_STANDARD 17)

# dependencies
find_library(LOG4CXX NAMES log4cxx)
find_path(LOG4CXX_INCLUDES NAMES log4cxx/log4cxx.h)
find_library(FMT NAMES fmt)
find_path(FMT_INCLUDES NAMES fmt/format.h)
find_path(CATCH2_INCLUDES catch2)

include_directories(include ${LOG4CXX_INCLUDES} ${FMT_INCLUDES} ${CATCH2_INCLUDES})
add_library(base SHARED src/Logger.cpp include/Logger.h src/services/SchedulerService.cpp include/services/SchedulerService.h
        include/File.h src/File.cpp include/Directory.h src/Directory.cpp include/models/Property.h
        include/models/PropertyBase.h include/services/PropertyService.h src/services/PropertyService.cpp
        include/services/StringifyService.h tests/models/PropertyTest.cpp include/utils/TypeName.h
        include/exceptions/PropertyNotFoundException.h src/models/Property.cpp include/factories/PropertyFactory.h src/factories/PropertyFactory.cpp include/services/AbstractService.h include/repositories/PropertyRepository.h src/repositories/PropertyRepository.cpp)
target_link_libraries(base ${LOG4CXX} ${FMT})

# Examples
ADD_SUBDIRECTORY(examples)
ADD_SUBDIRECTORY(tests)

# configuration of default path
set(CONFIG_DIRECTORY "${PROJECT_SOURCE_DIR}/cfg")
set(BINARY_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(LOG_DIRECTORY "${PROJECT_SOURCE_DIR}/log")

configure_file(
        ${PROJECT_SOURCE_DIR}/include/config.h.in
        ${PROJECT_SOURCE_DIR}/include/config.h
)