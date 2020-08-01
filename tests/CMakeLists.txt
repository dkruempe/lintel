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

conan_cmake_run(REQUIRES catch2/2.13.0
        OPTIONS
        ${CONAN_EXTRA_OPTIONS}
        BASIC_SETUP
        CMAKE_TARGETS # individual targets to link to
        BUILD
        missing)

include_directories(${CONAN_INCLUDE_DIRS})

ADD_EXECUTABLE(PropertyTest models/PropertyTest.cpp)
TARGET_LINK_LIBRARIES(PropertyTest kruempelmann::base_library)
ADD_TEST(propertyTest PropertyTest)

ADD_EXECUTABLE(PropertyFactoryTest factories/PropertyFactoryTest.cpp)
TARGET_LINK_LIBRARIES(PropertyFactoryTest kruempelmann::base_library)
ADD_TEST(propertyFactoryTest PropertyFactoryTest)

ADD_EXECUTABLE(PropertyServiceTest services/PropertyServiceTest.cpp)
TARGET_LINK_LIBRARIES(PropertyServiceTest kruempelmann::base_library)
ADD_TEST(propertyServiceTest PropertyServiceTest)