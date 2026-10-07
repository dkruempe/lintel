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
    integration/PostgreSqlIntegrationTest.cpp
    integration/ConfigIntegrationTest.cpp
    integration/HttpIntegrationTest.cpp
    integration/PropertySerializationTest.cpp
    core/CoreUtilsTest.cpp
    core/CursorTest.cpp
    core/RegexUtilsTest.cpp
    features/ConfigComponentsTest.cpp
    features/HistoryComponentTest.cpp
    features/HistoryServiceGatingTest.cpp
    external/HypodermicSignalTest.cpp
    events/EventBusTest.cpp
    plugins/SharedMemoryBootstrapPluginTest.cpp
    plugins/SingleInstanceBootstrapPluginTest.cpp
    cli/CliParserTest.cpp
    services/BootstrapServiceMockTest.cpp
    services/AuthServiceTest.cpp
    services/MessageQueueServiceMockTest.cpp
    services/MessageQueueOverflowTest.cpp
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
# Split the single-test-binary unity batch so multiple cores in CI actually get used
set_target_properties(base_tests PROPERTIES UNITY_BUILD_BATCH_SIZE 8)
target_compile_features(base_tests PUBLIC cxx_std_17)

# Stand-in for the system `sleep` utility, which ProcessServiceTest needs as a child process that
# stays alive. Depending on a system package made 16 test cases SKIP silently when it was missing;
# building it here removes the dependency. It lands in bin/ next to base_tests, like every other
# binary in this project (CMAKE_RUNTIME_OUTPUT_DIRECTORY).
ADD_EXECUTABLE(base_test_sleeper helpers/TestSleeper.cpp)
target_compile_features(base_test_sleeper PRIVATE cxx_std_17)
target_link_libraries(base_test_sleeper PRIVATE
    project_warnings
    lintel_options
)

# Hand the test binary the exact path of the helper. CMake knows it, so no PATH lookup and no
# guessing from argv[0]/cwd is needed; findSleepBinary() still falls back to a sibling binary and
# then to the system `sleep` if the file is gone.
add_dependencies(base_tests base_test_sleeper)
target_compile_definitions(base_tests PRIVATE
    BASE_TEST_SLEEPER_BINARY="$<TARGET_FILE:base_test_sleeper>")

target_link_libraries(base_tests PRIVATE
    lintel::lintel
    Catch2::Catch2WithMain
    trompeloeil::trompeloeil
    # Same warning set (incl. -Werror) as the library itself: `project_warnings` is
    # linked PRIVATE into lintel::lintel, so its INTERFACE options do
    # not reach this target — tests were compiled with plain defaults and CI could
    # not fail on a warning here.
    project_warnings
    lintel_options
)

# Auto-register each TEST_CASE / SECTION as a separate CTest test
catch_discover_tests(base_tests PROPERTIES LABELS "unit")
