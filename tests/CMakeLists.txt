cmake_minimum_required(VERSION 3.16)
project(base_examples)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

enable_testing()
#dependencies
if (NOT TARGET kruempelmann::base_library)
    find_package(base_library REQUIRED)
    include_directories(${base_library_INCLUDE_DIRS})
endif ()
find_package(Catch2 REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(magic_enum REQUIRED)
find_package(Boost REQUIRED)
find_package(date REQUIRED)

# property Test
ADD_EXECUTABLE(propertyTest models/PropertyTest.cpp)
target_compile_features(propertyTest PUBLIC cxx_std_17)
target_include_directories(propertyTest PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(propertyTest PRIVATE kruempelmann::base_library Catch2::Catch2 ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL Threads::Threads date::date)
ADD_TEST(NAME BaseLibrary.PropertyTest COMMAND propertyTest)

# propertyFactory Test
ADD_EXECUTABLE(propertyFactoryTest factories/PropertyFactoryTest.cpp)
target_compile_features(propertyFactoryTest PUBLIC cxx_std_17)
target_include_directories(propertyFactoryTest PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(propertyFactoryTest PRIVATE kruempelmann::base_library Catch2::Catch2 ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL Threads::Threads date::date)
ADD_TEST(NAME BaseLibrary.PropertyFactoryTest COMMAND propertyFactoryTest)

# property Service Test
ADD_EXECUTABLE(propertyServiceTest services/PropertyServiceTest.cpp)
target_compile_features(propertyServiceTest PUBLIC cxx_std_17)
target_include_directories(propertyServiceTest PRIVATE ${LOG4CXX_INCLUDES} ${magic_enum_INCLUDE_DIRS})
target_link_libraries(propertyServiceTest PRIVATE kruempelmann::base_library Catch2::Catch2 ${LOG4CXX} fmt::fmt tinyxml2::tinyxml2 PostgreSQL::PostgreSQL Threads::Threads date::date)
ADD_TEST(NAME BaseLibrary.PropertyServiceTest COMMAND propertyServiceTest)


ADD_EXECUTABLE(ProcessServiceTest services/ProcessServiceTest.cpp)
target_compile_features(ProcessServiceTest PUBLIC cxx_std_17)
target_include_directories(ProcessServiceTest PRIVATE ${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(ProcessServiceTest PRIVATE kruempelmann::base_library Catch2::Catch2 Threads::Threads date::date)
ADD_TEST(NAME BaseLibrary.ProcessServiceTest COMMAND ProcessServiceTest)