cmake_minimum_required(VERSION 3.16)
project(base_tests)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

enable_testing()
#dependencies
if (NOT TARGET kruempelmann::base_library)
    find_package(base_library REQUIRED)
    include_directories(${base_library_INCLUDE_DIRS})
endif ()
find_package(Catch2 REQUIRED)
find_package(trompeloeil REQUIRED)

# property Test
ADD_EXECUTABLE(propertyTest models/PropertyTest.cpp)
target_compile_features(propertyTest PUBLIC cxx_std_17)
target_link_libraries(propertyTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.PropertyTest COMMAND propertyTest)

# propertyFactory Test
ADD_EXECUTABLE(propertyFactoryTest factories/PropertyFactoryTest.cpp)
target_compile_features(propertyFactoryTest PUBLIC cxx_std_17)
target_link_libraries(propertyFactoryTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.PropertyFactoryTest COMMAND propertyFactoryTest)

# property Service Test
ADD_EXECUTABLE(propertyServiceTest services/PropertyServiceTest.cpp)
target_compile_features(propertyServiceTest PUBLIC cxx_std_17)
target_link_libraries(propertyServiceTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.PropertyServiceTest COMMAND propertyServiceTest)

# Integration Tests

# SQLite Integration Test
ADD_EXECUTABLE(sqliteIntegrationTest integration/SQLiteIntegrationTest.cpp)
target_compile_features(sqliteIntegrationTest PUBLIC cxx_std_17)
target_link_libraries(sqliteIntegrationTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.SQLiteIntegrationTest COMMAND sqliteIntegrationTest)

# Configuration Integration Test
ADD_EXECUTABLE(configIntegrationTest integration/ConfigIntegrationTest.cpp)
target_compile_features(configIntegrationTest PUBLIC cxx_std_17)
target_link_libraries(configIntegrationTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.ConfigIntegrationTest COMMAND configIntegrationTest)

# HTTP Integration Test
ADD_EXECUTABLE(httpIntegrationTest integration/HttpIntegrationTest.cpp)
target_compile_features(httpIntegrationTest PUBLIC cxx_std_17)
target_link_libraries(httpIntegrationTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.HttpIntegrationTest COMMAND httpIntegrationTest)

# Property Serialization Test
ADD_EXECUTABLE(propertySerializationTest integration/PropertySerializationTest.cpp)
target_compile_features(propertySerializationTest PUBLIC cxx_std_17)
target_link_libraries(propertySerializationTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.PropertySerializationTest COMMAND propertySerializationTest)

# Core Utils Test
ADD_EXECUTABLE(coreUtilsTest core/CoreUtilsTest.cpp)
target_compile_features(coreUtilsTest PUBLIC cxx_std_17)
target_link_libraries(coreUtilsTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.CoreUtilsTest COMMAND coreUtilsTest)

# Config Components Test
ADD_EXECUTABLE(configComponentsTest features/ConfigComponentsTest.cpp)
target_compile_features(configComponentsTest PUBLIC cxx_std_17)
target_link_libraries(configComponentsTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.ConfigComponentsTest COMMAND configComponentsTest)

# CLI Parser Test
ADD_EXECUTABLE(cliParserTest cli/CliParserTest.cpp)
target_compile_features(cliParserTest PUBLIC cxx_std_17)
target_link_libraries(cliParserTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain)
ADD_TEST(NAME BaseLibrary.CliParserTest COMMAND cliParserTest)

# Mock-based Unit Tests

# Bootstrap Service Mock Test
ADD_EXECUTABLE(bootstrapServiceMockTest services/BootstrapServiceMockTest.cpp)
target_compile_features(bootstrapServiceMockTest PUBLIC cxx_std_17)
target_link_libraries(bootstrapServiceMockTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain trompeloeil::trompeloeil)
ADD_TEST(NAME BaseLibrary.BootstrapServiceMockTest COMMAND bootstrapServiceMockTest)

# CLI CommandLineComponent Mock Test
ADD_EXECUTABLE(commandLineComponentMockTest cli/CommandLineComponentMockTest.cpp)
target_compile_features(commandLineComponentMockTest PUBLIC cxx_std_17)
target_link_libraries(commandLineComponentMockTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain trompeloeil::trompeloeil)
ADD_TEST(NAME BaseLibrary.CommandLineComponentMockTest COMMAND commandLineComponentMockTest)

# HTTP Controller Mock Test
ADD_EXECUTABLE(controllerMockTest http/ControllerMockTest.cpp)
target_compile_features(controllerMockTest PUBLIC cxx_std_17)
target_link_libraries(controllerMockTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain trompeloeil::trompeloeil)
ADD_TEST(NAME BaseLibrary.ControllerMockTest COMMAND controllerMockTest)

# MessageQueue Service Mock Test
ADD_EXECUTABLE(messageQueueServiceMockTest services/MessageQueueServiceMockTest.cpp)
target_compile_features(messageQueueServiceMockTest PUBLIC cxx_std_17)
target_link_libraries(messageQueueServiceMockTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain trompeloeil::trompeloeil)
ADD_TEST(NAME BaseLibrary.MessageQueueServiceMockTest COMMAND messageQueueServiceMockTest)

# AuthCliService Mock Test
ADD_EXECUTABLE(authCliServiceMockTest cli/AuthCliServiceMockTest.cpp)
target_compile_features(authCliServiceMockTest PUBLIC cxx_std_17)
target_link_libraries(authCliServiceMockTest PRIVATE kruempelmann::base_library Catch2::Catch2WithMain trompeloeil::trompeloeil)
ADD_TEST(NAME BaseLibrary.AuthCliServiceMockTest COMMAND authCliServiceMockTest)
