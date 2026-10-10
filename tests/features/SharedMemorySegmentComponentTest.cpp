#include <lintel/features/base/configuration/ConfigurationException.h>
#include <lintel/features/base/configuration/EnvironmentConfiguration.h>
#include <lintel/features/base/configuration/SharedMemorySegmentComponent.h>
#include <lintel/features/base/configuration/SharedMemorySegmentEntry.h>

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

std::filesystem::path uniqueSegmentDirectory()
{
  static int counter = 0;
  return std::filesystem::temp_directory_path() / ("shm_segment_component_test_" + std::to_string(counter++) + ".d");
}

// m_configDirectory must be the first member: members initialize in declaration order, so the guard
// sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration, and it is
// destroyed last - after the test case is done with it.
struct SharedMemorySegmentFixture
{
  ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_segment_cfg" };
  std::filesystem::path directory;
  std::shared_ptr<EnvironmentConfiguration> envConfig;

  SharedMemorySegmentFixture() : directory(uniqueSegmentDirectory())
  {
    envConfig = std::make_shared<EnvironmentConfiguration>();
  }

  ~SharedMemorySegmentFixture() { std::filesystem::remove_all(directory); }
};

}// namespace

// A <Path> element with an empty path attribute is rejected by processPathElement(). Falling through
// would not change the outcome - validateAndCreatePath() rejects an empty path as well - so the
// error message is what pins the guard that actually fired.
TEST_CASE("SharedMemorySegmentComponent: throw on empty Path path attribute")
{
  SharedMemorySegmentFixture fixture;
  SharedMemorySegmentComponent component(fixture.envConfig);

  std::string xml = R"(<SharedMemorySegments>
        <Path path=""/>
    </SharedMemorySegments>)";

  REQUIRE_THROWS_WITH(component.parse(xml, "test.xml", 0), Catch::Matchers::ContainsSubstring("wrong configured path"));
  REQUIRE_FALSE(std::filesystem::exists(fixture.directory));
}

// Without a <Path> element at all the path stays default constructed. Without the guard in
// validateAndCreatePath() the component would call create_directories("") and leak a
// std::filesystem_error instead of a ConfigurationException.
TEST_CASE("SharedMemorySegmentComponent: throw when the Path element is missing")
{
  SharedMemorySegmentFixture fixture;
  SharedMemorySegmentComponent component(fixture.envConfig);

  std::string xml = R"(<SharedMemorySegments>
        <SharedMemorySegment name="shm_test" size="1kB"/>
    </SharedMemorySegments>)";

  REQUIRE_THROWS_AS(component.parse(xml, "test.xml", 0), ConfigurationException);
  REQUIRE_THROWS_WITH(
    component.parse(xml, "test.xml", 0), Catch::Matchers::ContainsSubstring("Path definition is completely missing"));
  REQUIRE_FALSE(std::filesystem::exists(fixture.directory));
}

// Counterpart of the two cases above: a configured path is still created and used, so the throwing
// cases above are not passing because the component is broken in a different way.
TEST_CASE("SharedMemorySegmentComponent: parses Path and SharedMemorySegment")
{
  SharedMemorySegmentFixture fixture;
  SharedMemorySegmentComponent component(fixture.envConfig);

  std::string xml = R"(<SharedMemorySegments>
        <Path path=")"
                    + fixture.directory.string() + R"("/>
        <SharedMemorySegment name="shm_test" size="1kB"/>
    </SharedMemorySegments>)";

  auto entries = component.parse(xml, "test.xml", 0);
  REQUIRE(entries.size() == 2);
  REQUIRE(std::filesystem::exists(fixture.directory));

  auto pathEntry = std::static_pointer_cast<SharedMemorySegmentEntry>(entries[0]);
  REQUIRE(*pathEntry->getSharedMemoryPath() == fixture.directory);

  auto segmentEntry = std::static_pointer_cast<SharedMemorySegmentEntry>(entries[1]);
  REQUIRE(segmentEntry->getSharedMemorySegment()->getName() == "shm_test");
  REQUIRE(segmentEntry->getSharedMemorySegment()->getSize() == 1000);
}