#include <lintel/core/configuration/Configuration.h>
#include <lintel/core/configuration/EnvironmentConfiguration.h>
#include <lintel/core/models/ProcessName.h>
#include <lintel/features/base/configuration/SharedMemorySegmentComponent.h>
#include <lintel/features/base/configuration/SharedMemorySegmentEntry.h>
#include <lintel/features/base/services/SchedulerService.h>
#include <lintel/features/base/services/SharedMemorySegmentManager.h>
#include <lintel/features/base/services/SharedMemoryService.h>
#include <lintel/features/base/services/ShmConstructors.h>
#include <lintel/features/property/repositories/SharedMemoryPropertyRepository.h>

#include <catch2/catch_all.hpp>

#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace lintel {
namespace {

  std::filesystem::path uniquePropertySizeShmPath()
  {
    static std::uint64_t counter = 0;
    return std::filesystem::temp_directory_path()
           / ("property_size_test_" + std::to_string(getpid()) + "_" + std::to_string(counter++) + ".bin");
  }

  /** Wires a real shared memory property repository over a unique segment. Named apart from the
   *  identically shaped fixture in PropertyChangeTest.cpp so the two can share a unity chunk. */
  struct PropertySizeRepositoryFixture
  {
    std::shared_ptr<Configuration> configuration;
    std::shared_ptr<ProcessName> processName;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemorySegmentManager> segmentManager;
    std::shared_ptr<SharedMemoryService> sharedMemoryService;
    std::shared_ptr<SharedMemoryPropertyRepository> repository;
    std::filesystem::path path;

    PropertySizeRepositoryFixture() : path(uniquePropertySizeShmPath())
    {
      auto environmentConfiguration = std::make_shared<EnvironmentConfiguration>();
      environmentConfiguration->overrides(
        EnvironmentConfiguration::ConfigDirectory, std::filesystem::temp_directory_path().string() + "/nonexistent");
      configuration =
        std::make_shared<Configuration>(std::vector<std::shared_ptr<Component>>{}, std::move(environmentConfiguration));
      configuration->setEntries({ std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(),
        std::make_shared<SharedMemorySegment>(path, "shm_property", 5 * 1024 * 1024)) });
      processName = std::make_shared<ProcessName>("testProcess");
      scheduler = std::make_shared<SchedulerService>(processName);
      segmentManager = std::make_shared<SharedMemorySegmentManager>(configuration);
      sharedMemoryService = std::make_shared<SharedMemoryService>(segmentManager, scheduler, processName);
      repository = std::make_shared<SharedMemoryPropertyRepository>(
        sharedMemoryService, segmentManager, configuration, processName);
    }

    ~PropertySizeRepositoryFixture()
    {
      repository.reset();
      sharedMemoryService.reset();
      segmentManager.reset();
      std::filesystem::remove(path);
    }
  };

}// namespace

// PropertyDataDto::getSize() is a hand-maintained size hint for the shared memory map. It is not
// validated anywhere in the production code: SharedMemoryPropertyRepository hands it to the base
// class as-is, and only a later startup notices a drift between it and sizeof(PropertyDataDto) -
// after the segment has already been mapped with the wrong element size. Pin the value here.
//
// shm::String is a boost::container::basic_string with a boost::interprocess allocator, i.e.
// 8 bytes size + 8 bytes data pointer + 16 bytes allocator on the 64-bit LP64 ABI used by CI.
TEST_CASE("PropertyDataDto::getSize matches the mapped element size")
{
  REQUIRE(sizeof(shm::String) == 32);
  REQUIRE(PropertyDataDto::getSize() == 6 * 32);
  REQUIRE(PropertyDataDto::getSize() == 192);

  // The hint has to cover the DTO exactly - one string too few would let the map read past a value.
  REQUIRE(PropertyDataDto::getSize() == static_cast<int32_t>(sizeof(shm::String) * 6));
  REQUIRE(PropertyDataDto::getSize() == static_cast<int32_t>(sizeof(PropertyDataDto)));

  PropertySizeRepositoryFixture fixture;

  // And the hint has to be the one the repository actually uses - not just a correct formula.
  REQUIRE(fixture.repository->getSizeOfData() == static_cast<std::size_t>(PropertyDataDto::getSize()));
  REQUIRE(fixture.repository->getSizeOfData() == sizeof(PropertyDataDto));
}

TEST_CASE("PropertyDataDto compares lexicographically over all fields")
{
  PropertySizeRepositoryFixture fixture;

  // reconstruct the segment handle by name so strings can be allocated in the real segment
  auto segment = std::make_shared<SharedMemorySegment>(fixture.path, "shm_property", 5 * 1024 * 1024);
  auto make = [&](const std::string &processName,
                const std::string &className,
                const std::string &instanceName,
                const std::string &name,
                const std::string &value,
                const std::string &type) {
    return PropertyDataDto{ shm::constructString(*fixture.sharedMemoryService, segment, processName),
      shm::constructString(*fixture.sharedMemoryService, segment, className),
      shm::constructString(*fixture.sharedMemoryService, segment, instanceName),
      shm::constructString(*fixture.sharedMemoryService, segment, name),
      shm::constructString(*fixture.sharedMemoryService, segment, value),
      shm::constructString(*fixture.sharedMemoryService, segment, type) };
  };

  const PropertyDataDto base = make("p", "c", "i", "n", "v", "t");
  const PropertyDataDto identical = make("p", "c", "i", "n", "v", "t");
  REQUIRE(base == identical);
  REQUIRE_FALSE(base != identical);
  REQUIRE(base <= identical);
  REQUIRE(base >= identical);
  REQUIRE_FALSE(base < identical);
  REQUIRE_FALSE(base > identical);

  const std::vector<PropertyDataDto> greater = {
    make("q", "c", "i", "n", "v", "t"),// processName decides
    make("p", "d", "i", "n", "v", "t"),// className decides
    make("p", "c", "j", "n", "v", "t"),// instanceName decides
    make("p", "c", "i", "o", "v", "t"),// name decides
    make("p", "c", "i", "n", "w", "t"),// value decides
    make("p", "c", "i", "n", "v", "u"),// type decides
  };
  for (const auto &other : greater) {
    REQUIRE(base < other);
    REQUIRE(base <= other);
    REQUIRE(other > base);
    REQUIRE(other >= base);
    REQUIRE(base != other);
    REQUIRE_FALSE(base == other);
  }
}
}// namespace lintel