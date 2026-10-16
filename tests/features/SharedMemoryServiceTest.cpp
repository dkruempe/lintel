#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/exceptions/ShmSegmentNotFound.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/models/SharedMemorySegment.h"
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

  /** All page-aligned so boost's grow/shrink arithmetic is exact. */
  constexpr std::size_t kInitialSize = 1u << 20;
  constexpr std::size_t kGrowSize = 1u << 20;
  constexpr std::size_t kMaxSize = 4u << 20;

  std::filesystem::path uniqueSegmentPath()
  {
    static unsigned int counter = 0;
    const auto path = std::filesystem::temp_directory_path()
                      / ("shm_service_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
    std::filesystem::remove(path);
    return path;
  }

  // macOS caps POSIX semaphore names at 31 characters (PSEMNAMLEN) and
  // SharedMemoryService derives one as "<segment name>_remap_sem", so keep the
  // segment name short enough for the suffix to fit (<= 21 characters).
  std::string uniqueSegmentName()
  {
    static unsigned int counter = 0;
    return "shm_svc_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  // m_configDirectory must be the first member: members initialize in declaration order, so the
  // guard sets CONFIG_DIRECTORY before the constructor body builds the EnvironmentConfiguration.
  struct SharedMemoryServiceFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_service_cfg" };
    std::filesystem::path path;
    std::string name;
    std::shared_ptr<SharedMemorySegment> segment;
    std::shared_ptr<SharedMemoryService> service;
    std::shared_ptr<SchedulerService> scheduler;

    explicit SharedMemoryServiceFixture(std::size_t size = kInitialSize, std::size_t maxSize = 0)
      : path(uniqueSegmentPath()), name(uniqueSegmentName())
    {
      if (maxSize != 0) {
        segment = std::make_shared<SharedMemorySegment>(
          path, name, size, SharedMemorySegment::AutoExtendConfig{ kGrowSize, maxSize });
      } else {
        segment = std::make_shared<SharedMemorySegment>(path, name, size);
      }

      auto config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });

      auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
      // The scheduler is deliberately not initialised (no worker threads): growOf/shrinkOf are
      // synchronous and only onCheck() - driven by scheduler tasks - needs threads.
      scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-service-test"));
      service = std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("shm-service-test"));
    }

    ~SharedMemoryServiceFixture()
    {
      // Close the mapping/named semaphore before unlinking them.
      service.reset();
      scheduler.reset();
      std::filesystem::remove(path);
      boost::interprocess::named_semaphore::remove((name + "_remap_sem").c_str());
    }

    [[nodiscard]] std::size_t currentSize() const { return service->showStateOf(segment).getCurrentSize(); }
  };

}// namespace

TEST_CASE("SharedMemoryService: grow increases the segment size and bumps the generation")
{
  SharedMemoryServiceFixture fixture;
  REQUIRE(fixture.service->getGeneration() == 0);
  const std::size_t before = fixture.currentSize();

  fixture.service->growOf(fixture.segment, kGrowSize);

  REQUIRE(fixture.currentSize() == before + kGrowSize);
  REQUIRE(fixture.service->getGeneration() == 1);
}

TEST_CASE("SharedMemoryService: grow by zero is a no-op")
{
  SharedMemoryServiceFixture fixture;
  const std::size_t before = fixture.currentSize();

  fixture.service->growOf(fixture.segment, 0);

  REQUIRE(fixture.currentSize() == before);
  REQUIRE(fixture.service->getGeneration() == 0);
}

TEST_CASE("SharedMemoryService: grow of an unknown segment throws ShmSegmentNotFound")
{
  SharedMemoryServiceFixture fixture;
  auto unknown = std::make_shared<SharedMemorySegment>(fixture.path, fixture.name + "_missing", kInitialSize);

  REQUIRE_THROWS_AS(fixture.service->growOf(unknown, kGrowSize), ShmSegmentNotFound);
}

TEST_CASE("SharedMemoryService: grow is capped at the configured maximum size")
{
  SharedMemoryServiceFixture fixture(kInitialSize, kMaxSize);

  // Ask for more than the headroom - the request is capped instead of overshooting the limit.
  fixture.service->growOf(fixture.segment, 100 * kGrowSize);

  REQUIRE(fixture.currentSize() == kMaxSize);
  REQUIRE(fixture.service->getGeneration() == 1);

  // A further grow is refused once the segment is at its maximum.
  REQUIRE_THROWS_AS(fixture.service->growOf(fixture.segment, kGrowSize), std::runtime_error);
  REQUIRE(fixture.service->getGeneration() == 1);
}

TEST_CASE("SharedMemoryService: shrink reduces the segment size and bumps the generation")
{
  SharedMemoryServiceFixture fixture;
  fixture.service->growOf(fixture.segment, 4 * kGrowSize);
  const std::size_t grown = fixture.currentSize();
  const std::size_t generation = fixture.service->getGeneration();

  fixture.service->shrinkOf(fixture.segment);

  REQUIRE(fixture.currentSize() <= grown);
  REQUIRE(fixture.service->getGeneration() == generation + 1);
}

TEST_CASE("SharedMemoryService: shrink of an unknown segment throws ShmSegmentNotFound")
{
  SharedMemoryServiceFixture fixture;
  auto unknown = std::make_shared<SharedMemorySegment>(fixture.path, fixture.name + "_missing", kInitialSize);

  REQUIRE_THROWS_AS(fixture.service->shrinkOf(unknown), ShmSegmentNotFound);
}

TEST_CASE("SharedMemoryService: showStateOf reports a sane segment")
{
  SharedMemoryServiceFixture fixture;

  const auto info = fixture.service->showStateOf(fixture.segment);

  REQUIRE(info.getCurrentSize() >= kInitialSize);
  REQUIRE(info.getFreeSize() <= info.getCurrentSize());
  REQUIRE(info.isSanity());
}

TEST_CASE("SegmentSemaphoreGuard: times out instead of blocking on a crashed holder")
{
  static unsigned int counter = 0;
  const std::string name = "shm_guard_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  boost::interprocess::named_semaphore semaphore(boost::interprocess::open_or_create, name.c_str(), 1u);

  {
    // The first guard owns the (binary) semaphore, simulating a process that
    // crashed while holding it and never posted.
    shm::SegmentSemaphoreGuard holder(semaphore);
    REQUIRE_THROWS_AS(shm::SegmentSemaphoreGuard(semaphore, std::chrono::milliseconds(50)), std::runtime_error);
  }

  // After the holder released it the semaphore is usable again.
  REQUIRE_NOTHROW(shm::SegmentSemaphoreGuard(semaphore, std::chrono::milliseconds(50)));
  boost::interprocess::named_semaphore::remove(name.c_str());
}

TEST_CASE("SharedMemoryService: periodic check leaves a non auto-extend segment untouched")
{
  SharedMemoryServiceFixture fixture;

  fixture.scheduler->onInitialize();
  fixture.service->onInitialize();

  // default schedule rate is 2s - give the scheduled onCheck a chance to run
  std::this_thread::sleep_for(std::chrono::milliseconds(2600));

  fixture.scheduler->onShutdown();
  REQUIRE(fixture.service->getGeneration() == 0);
}

TEST_CASE("SharedMemoryService: periodic check auto-extends a nearly full segment")
{
  SharedMemoryServiceFixture fixture(kInitialSize, kMaxSize);
  REQUIRE(fixture.segment->isAutoExtend());

  // drain the segment until less than the auto-extend epsilon (1000 bytes) is free
  auto accessor = fixture.service->getSegmentAccessor(fixture.segment);
  while (fixture.service->showStateOf(fixture.segment).getFreeSize() >= 1000) {
    try {
      static_cast<void>(accessor.allocate(64));
    } catch (const std::exception &) {
      break;
    }
  }
  REQUIRE(fixture.service->showStateOf(fixture.segment).getFreeSize() < 1000);

  fixture.scheduler->onInitialize();
  fixture.service->onInitialize();

  std::this_thread::sleep_for(std::chrono::milliseconds(2600));

  fixture.scheduler->onShutdown();
  REQUIRE(fixture.service->getGeneration() >= 1);
  REQUIRE(fixture.currentSize() == kInitialSize + kGrowSize);
}

TEST_CASE("SharedMemoryService: getSegmentAccessor reports allocation through the segment")
{
  SharedMemoryServiceFixture fixture;

  auto accessor = fixture.service->getSegmentAccessor(fixture.segment);
  void *block = accessor.allocate(128);
  REQUIRE(block != nullptr);
  accessor.deallocate(block);

  // the same accessor is returned again for the same segment
  auto second = fixture.service->getSegmentAccessor(fixture.segment);
  void *secondBlock = second.allocate(64);
  REQUIRE(secondBlock != nullptr);
  second.deallocate(secondBlock);
}

TEST_CASE("SharedMemoryService: getSegmentAccessor throws for an unknown segment")
{
  SharedMemoryServiceFixture fixture;
  auto unknown = std::make_shared<SharedMemorySegment>(fixture.path, fixture.name + "_missing", kInitialSize);

  REQUIRE_THROWS_AS(fixture.service->getSegmentAccessor(unknown), ShmSegmentNotFound);
}

}// namespace lintel
