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

conan_cmake_run(REQUIRES catch2/2.13.0 fmt/7.0.3 tinyxml2/7.1.0 magic_enum/0.7.0
        OPTIONS
        ${CONAN_EXTRA_OPTIONS}
        BASIC_SETUP
        CMAKE_TARGETS # individual targets to link to
        BUILD
        missing)

# property Test
ADD_EXECUTABLE(propertyTest models/PropertyTest.cpp)
target_compile_features(propertyTest PUBLIC cxx_std_17)
target_include_directories(propertyTest PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(propertyTest PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 pthread)
ADD_TEST(NAME BaseLibrary.PropertyTest COMMAND propertyTest)

# propertyFactory Test
ADD_EXECUTABLE(propertyFactoryTest factories/PropertyFactoryTest.cpp)
target_compile_features(propertyFactoryTest PUBLIC cxx_std_17)
target_include_directories(propertyFactoryTest PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(propertyFactoryTest PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 pthread)
ADD_TEST(NAME BaseLibrary.PropertyFactoryTest COMMAND propertyFactoryTest)

# property Service Test
ADD_EXECUTABLE(propertyServiceTest services/PropertyServiceTest.cpp)
target_compile_features(propertyServiceTest PUBLIC cxx_std_17)
target_include_directories(propertyServiceTest PRIVATE ${CONAN_INCLUDE_DIRS} ${LOG4CXX_INCLUDES})
target_link_libraries(propertyServiceTest PRIVATE kruempelmann::base_library ${LOG4CXX} CONAN_PKG::fmt CONAN_PKG::tinyxml2 pthread)
ADD_TEST(NAME BaseLibrary.PropertyServiceTest COMMAND propertyServiceTest)