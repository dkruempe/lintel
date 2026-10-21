#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/shm/ShmView.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/SharedMemorySegmentComponent.h"
#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryService.h"
#include "lintel/features/base/services/ShmConstructors.h"

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  constexpr std::size_t kViewSegmentSize = 1u << 20;

  std::filesystem::path uniqueViewPath()
  {
    static unsigned int counter = 0;
    return std::filesystem::temp_directory_path()
           / ("shm_view_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
  }

  std::string uniqueViewSegmentName()
  {
    static unsigned int counter = 0;
    return "shm_view_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  struct ShmViewFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_view_cfg" };
    std::filesystem::path path;
    std::string segmentName;
    std::shared_ptr<SharedMemorySegment> segment;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemoryService> service;

    ShmViewFixture() : path(uniqueViewPath()), segmentName(uniqueViewSegmentName())
    {
      segment = std::make_shared<SharedMemorySegment>(path, segmentName, kViewSegmentSize);
      auto config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
      auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
      scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-view-test"));
      service = std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("shm-view-test"));
    }

    ~ShmViewFixture()
    {
      service.reset();
      scheduler.reset();
      std::filesystem::remove(path);
      boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(segmentName).c_str());
      SegmentLockOwner::removeOf(segmentName);
    }

    /** Create the named array the views point at. */
    std::array<int, 4> &createValues(const std::string &name = "values")
    {
      auto &values = shm::constructArray<int, 4>(*service, segment, name);
      values = { 10, 20, 30, 40 };
      return values;
    }
  };

}// namespace

TEST_CASE("ShmView: a valid view exposes the named elements")
{
  ShmViewFixture fixture;
  fixture.createValues();

  auto view = shm::viewOf<int>(fixture.service, fixture.segment, "values", 4);

  REQUIRE_FALSE(view.empty());
  REQUIRE(view.size() == 4);
  REQUIRE(view.sizeBytes() == 4 * sizeof(int));
  REQUIRE(view.generation() == fixture.service->getGeneration());
  REQUIRE(view.hasGenerationProvider());
  REQUIRE(view.valid());
  REQUIRE(view.valid(fixture.service->getGeneration()));
  REQUIRE_FALSE(view.isStale());

  REQUIRE(view.get() == view.data());
  REQUIRE(view[0] == 10);
  REQUIRE(view.at(1) == 20);
  REQUIRE(view.front() == 10);
  REQUIRE(view.back() == 40);

  int expected = 10;
  for (int value : view) {
    REQUIRE(value == expected);
    expected += 10;
  }

  REQUIRE_THROWS_AS(view.at(4), std::out_of_range);
  REQUIRE_THROWS_AS(view[4], std::out_of_range);
}

TEST_CASE("ShmView: an empty view is invalid")
{
  ShmView<int> view;

  REQUIRE(view.empty());
  REQUIRE(view.size() == 0);
  REQUIRE(view.get() == nullptr);
  REQUIRE_FALSE(view.valid());
  REQUIRE(view.isStale());
  REQUIRE_THROWS_AS(view.at(0), std::runtime_error);
  REQUIRE_THROWS_AS(view.front(), std::runtime_error);
}

TEST_CASE("ShmView: detects the remap of its segment through the generation")
{
  ShmViewFixture fixture;
  fixture.createValues();

  auto view = shm::viewOf<int>(fixture.service, fixture.segment, "values", 4);
  const std::size_t generation = view.generation();
  REQUIRE(view[2] == 30);

  fixture.service->growOf(fixture.segment, 1u << 20);

  REQUIRE(view.isStale());
  REQUIRE_FALSE(view.valid());
  REQUIRE(view.valid(generation));// the old generation still matches the stored one
  REQUIRE_FALSE(view.valid(fixture.service->getGeneration()));
  REQUIRE_THROWS_AS(view[2], std::runtime_error);
  REQUIRE_THROWS_AS(view.slice(0, 2), std::runtime_error);

  // Re-creating the view resolves the object in the new mapping.
  auto refreshed = shm::viewOf<int>(fixture.service, fixture.segment, "values", 4);
  REQUIRE(refreshed.valid());
  REQUIRE(refreshed[2] == 30);

  // The explicit service constructor installs the same provider.
  ShmView<int> direct(refreshed.data(), refreshed.size(), fixture.service);
  REQUIRE(direct.valid());
  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(direct.isStale());
  REQUIRE_THROWS_AS(direct.at(0), std::runtime_error);
}

TEST_CASE("ShmView: slices keep generation and provider")
{
  ShmViewFixture fixture;
  fixture.createValues();

  auto view = shm::viewOf<int>(fixture.service, fixture.segment, "values", 4);
  auto middle = view.slice(1, 2);

  REQUIRE(middle.size() == 2);
  REQUIRE(middle.generation() == view.generation());
  REQUIRE(middle.hasGenerationProvider());
  REQUIRE(middle[0] == 20);
  REQUIRE(middle[1] == 30);

  auto first = view.first(2);
  auto last = view.last(2);
  REQUIRE(first[0] == 10);
  REQUIRE(first[1] == 20);
  REQUIRE(last[0] == 30);
  REQUIRE(last[1] == 40);

  REQUIRE_THROWS_AS(view.slice(3, 2), std::out_of_range);
  REQUIRE_THROWS_AS(view.first(5), std::out_of_range);
  REQUIRE_THROWS_AS(view.last(5), std::out_of_range);

  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(middle.isStale());
  REQUIRE(first.isStale());
  REQUIRE(last.isStale());
}

TEST_CASE("ShmView: a manual view uses an explicit generation")
{
  int values[] = { 1, 2, 3 };
  ShmView<int> view(values, 3, 7);

  REQUIRE(view.valid(7));
  REQUIRE_FALSE(view.valid(8));
  REQUIRE(view[2] == 3);
  REQUIRE_FALSE(view.hasGenerationProvider());
}

TEST_CASE("ShmView: viewOf rejects unknown objects")
{
  ShmViewFixture fixture;

  REQUIRE_THROWS_AS(shm::viewOf<int>(fixture.service, fixture.segment, "missing", 1), std::out_of_range);
  REQUIRE_THROWS_AS(shm::viewOf<int>(nullptr, fixture.segment, "missing", 1), std::invalid_argument);
  REQUIRE_THROWS_AS(shm::viewOf<int>(fixture.service, nullptr, "missing", 1), std::invalid_argument);
}

}// namespace lintel
