cmake_minimum_required(VERSION 3.16)
project(base_tests)
cmake_policy(SET CMP0025 NEW)
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

enable_testing()
#dependencies
find_package(Catch2 REQUIRED)
find_package(trompeloeil REQUIRED)
include(CTest)
include(Catch)

# All tests in a single binary — one link instead of 27.
ADD_EXECUTABLE(base_tests
    models/PropertyTest.cpp
    factories/PropertyFactoryTest.cpp
    services/PropertyServiceTest.cpp
    property/PropertyChangeTest.cpp
    integration/SQLiteIntegrationTest.cpp
    integration/ConfigIntegrationTest.cpp
    integration/HttpIntegrationTest.cpp
    integration/PropertySerializationTest.cpp
    core/CoreUtilsTest.cpp
    core/CursorTest.cpp
    core/RegexUtilsTest.cpp
    features/ConfigComponentsTest.cpp
    features/HistoryComponentTest.cpp
    features/HistoryServiceGatingTest.cpp
    events/EventBusTest.cpp
    plugins/SharedMemoryBootstrapPluginTest.cpp
    plugins/SingleInstanceBootstrapPluginTest.cpp
    cli/CliParserTest.cpp
    services/BootstrapServiceMockTest.cpp
    services/AuthServiceTest.cpp
    services/MessageQueueServiceMockTest.cpp
    cli/CommandLineComponentMockTest.cpp
    cli/CommandLineHistoryServiceTest.cpp
    cli/AuthCliServiceMockTest.cpp
    cli/UserManagementCliComponentMockTest.cpp
    http/ControllerMockTest.cpp
    http/ClientIpResolverTest.cpp
    http/SharedMemoryControllerMockTest.cpp
    services/ProcessServiceTest.cpp
    repositories/UserGroupRepositoryTest.cpp
    repositories/UserRepositoryPagingTest.cpp
    repositories/MessageQueueRepositoryPagingTest.cpp
)
target_compile_features(base_tests PUBLIC cxx_std_17)
target_link_libraries(base_tests PRIVATE
    kruempelmann::base_library
    Catch2::Catch2WithMain
    trompeloeil::trompeloeil
)

# Auto-register each TEST_CASE / SECTION as a separate CTest test
catch_discover_tests(base_tests PROPERTIES LABELS "unit")
