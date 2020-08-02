cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)

enable_testing()
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

ADD_EXECUTABLE(propertyTest models/PropertyTest.cpp)
target_compile_features(propertyTest PUBLIC cxx_std_17)
TARGET_INCLUDE_DIRECTORIES(propertyTest PRIVATE ${CONAN_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(propertyTest PRIVATE kruempelmann::base_library)
ADD_TEST(NAME BaseLibrary.PropertyTest COMMAND propertyTest)

ADD_EXECUTABLE(propertyFactoryTest factories/PropertyFactoryTest.cpp)
target_compile_features(propertyFactoryTest PUBLIC cxx_std_17)
TARGET_INCLUDE_DIRECTORIES(propertyFactoryTest PRIVATE ${CONAN_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(propertyFactoryTest PRIVATE kruempelmann::base_library)
ADD_TEST(NAME BaseLibrary.PropertyFactoryTest COMMAND propertyFactoryTest)

ADD_EXECUTABLE(propertyServiceTest services/PropertyServiceTest.cpp)
target_compile_features(propertyServiceTest PUBLIC cxx_std_17)
TARGET_INCLUDE_DIRECTORIES(propertyServiceTest PRIVATE ${CONAN_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(propertyServiceTest PRIVATE kruempelmann::base_library)
ADD_TEST(NAME BaseLibrary.PropertyServiceTest COMMAND propertyServiceTest)