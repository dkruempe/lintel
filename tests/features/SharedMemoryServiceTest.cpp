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
#include "lintel/core/services/ReadinessRegistry.h"
#include "lintel/core/shm/SegmentLayoutPolicy.h"
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

  /** A process id that provably does not exist: 2^30 is above Linux's PID_MAX_LIMIT
   *  (2^22) and far above the Windows PID allocation granularity. */
  constexpr ProcessId kDeadProcessId = static_cast<ProcessId>(1) << 30;

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

    explicit SharedMemoryServiceFixture(std::size_t size = kInitialSize,
      std::size_t maxSize = 0,
      bool withOrphanedLock = false)
      : path(uniqueSegmentPath()), name(uniqueSegmentName())
    {
      if (maxSize != 0) {
        segment = std::make_shared<SharedMemorySegment>(
          path, name, size, SharedMemorySegment::AutoExtendConfig{ kGrowSize, maxSize });
      } else {
        segment = std::make_shared<SharedMemorySegment>(path, name, size);
      }

      if (withOrphanedLock) {
        // Exactly what a process that crashed while holding the remap lock
        // leaves behind: a semaphore nobody will ever post again plus a holder
        // record pointing at a process that no longer exists.
        boost::interprocess::named_semaphore semaphore(
          boost::interprocess::open_or_create, shm::remapSemaphoreNameOf(name).c_str(), 1u);
        REQUIRE(semaphore.try_wait());
        SegmentLockOwner sidecar(name);
        sidecar.set(kDeadProcessId);
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
      boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(name).c_str());
      // the holder record outlives the semaphore otherwise, and a recycled pid
      // of a later test run would then look like a living lock holder
      SegmentLockOwner::removeOf(name);
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
  // allow the file to be closed/reopened before fixture teardown; in particular on
  // Windows a still-mapped segment must be released before boost::interprocess
  // finalises the file or the fixture tries to remove it
  fixture.service.reset();
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

TEST_CASE("SharedMemoryService: onInitialize registers a readiness check for its segments")
{
  // the registry is process-wide; clear it so an earlier case in a direct run
  // cannot add checks of its own
  ReadinessRegistry::instance().clear();

  SharedMemoryServiceFixture fixture;
  fixture.scheduler->onInitialize();
  fixture.service->onInitialize();
  fixture.scheduler->onShutdown();

  const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
  REQUIRE(results.size() == 1);
  REQUIRE(results[0].m_name == "shared_memory");
  REQUIRE(results[0].m_ready);

  // the check follows the service lifetime: a stale registration must report
  // not ready instead of claiming the (gone) segments are mapped
  fixture.service.reset();
  const std::vector<ReadinessRegistry::CheckResult> stale = ReadinessRegistry::instance().run();
  REQUIRE(stale.size() == 1);
  REQUIRE_FALSE(stale[0].m_ready);
  REQUIRE_FALSE(stale[0].m_detail.empty());

  ReadinessRegistry::instance().clear();
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

TEST_CASE("SharedMemoryService: recover reclaims the lock of a holder that is gone")
{
  SharedMemoryServiceFixture fixture;
  // take the lock and never give it back - the state a crashed holder leaves
  // behind; the fresh handle below refers to the same semaphore object
  boost::interprocess::named_semaphore stuck(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(stuck.try_wait());

  SegmentLockOwner owner(fixture.name);
  REQUIRE_FALSE(processIsAlive(kDeadProcessId));
  owner.set(kDeadProcessId);

  REQUIRE(fixture.service->recover() == 1);

  // a new handle sees the recreated semaphore, so the next waiter gets the lock
  boost::interprocess::named_semaphore fresh(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(fresh.try_wait());
  // still binary: reclaiming must not hand out two locks at once
  REQUIRE_FALSE(fresh.try_wait());
  fresh.post();

  // the holder record is cleared with it, so a second run has nothing to do
  REQUIRE(fixture.service->recover() == 0);
}

TEST_CASE("SharedMemoryService: recover leaves the lock of a living holder alone")
{
  SharedMemoryServiceFixture fixture;
  boost::interprocess::named_semaphore stuck(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(stuck.try_wait());

  SegmentLockOwner owner(fixture.name);
  owner.set(currentProcessId());

  // taking it away from a process that still runs would give two processes
  // independent locks for one critical section
  REQUIRE(fixture.service->recover() == 0);
  REQUIRE_THROWS_AS(shm::SegmentSemaphoreGuard(stuck, std::chrono::milliseconds(50)), std::runtime_error);
}

TEST_CASE("SharedMemoryService: opening a segment over a dead holder recovers it at once")
{
  // the orphan exists before the service is built, so the recovery has to run
  // while the segment is opened - without it the very first lock acquisition of
  // this process would wait for the bounded timeout and then fail
  SharedMemoryServiceFixture fixture(kInitialSize, 0, true);

  REQUIRE(fixture.service->recover() == 0);
  REQUIRE_NOTHROW(fixture.service->getSegmentAccessor(fixture.segment));
  REQUIRE(fixture.service->getGeneration() == 0);
}

TEST_CASE("SharedMemoryService: recover keeps a free lock that only has a stale record")
{
  SharedMemoryServiceFixture fixture;
  boost::interprocess::named_semaphore free(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());

  SegmentLockOwner owner(fixture.name);
  owner.set(kDeadProcessId);

  // the holder died *after* it released the lock: the lock is free, so there is
  // nothing to reclaim and recreating it would orphan every open handle
  REQUIRE(fixture.service->recover() == 0);
  REQUIRE(owner.pid() == 0);

  REQUIRE(free.try_wait());
  // same object as the caller's: recovery did not replace it behind our back
  boost::interprocess::named_semaphore sameObject(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE_FALSE(sameObject.try_wait());
  free.post();
}

TEST_CASE("SharedMemoryService: recover reclaims a stuck lock that has no holder record")
{
  SharedMemoryServiceFixture fixture;
  // a process that crashed between taking the lock and recording itself
  boost::interprocess::named_semaphore stuck(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(stuck.try_wait());
  SegmentLockOwner owner(fixture.name);
  REQUIRE(owner.pid() == 0);

  REQUIRE(fixture.service->recover() == 1);

  boost::interprocess::named_semaphore fresh(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(fresh.try_wait());
  REQUIRE_FALSE(fresh.try_wait());
}

TEST_CASE("SharedMemoryService: releasing the lock keeps a record that is no longer ours")
{
  SharedMemoryServiceFixture fixture;
  SegmentLockOwner owner(fixture.name);
  auto &handle = fixture.service->segmentOf(fixture.name);
  {
    shm::SegmentSemaphoreGuard gate(handle, std::chrono::milliseconds(500));
    REQUIRE(owner.pid() == currentProcessId());
    // the next holder has recorded itself before this guard is destroyed
    owner.set(kDeadProcessId);
  }
  // an unconditional clear would wipe a live holder and the next recovery would
  // then reclaim a lock that is in use
  REQUIRE(owner.pid() == kDeadProcessId);
}

TEST_CASE("SharedMemory lock recovery: a recycled process id is not the same holder")
{
  const ProcessId self = currentProcessId();
  const std::int64_t ticks = processStartTicksOf(self);
  REQUIRE(ticks > 0);
  REQUIRE(processIdentityIsAlive(ProcessIdentity{ self, ticks }));
  // same id, different start time: the recorded holder is gone
  REQUIRE_FALSE(processIdentityIsAlive(ProcessIdentity{ self, ticks + 1 }));
  // unknown start time falls back to the id - never reclaim from a live process
  REQUIRE(processIdentityIsAlive(ProcessIdentity{ self, 0 }));
  REQUIRE_FALSE(processIdentityIsAlive(ProcessIdentity{ kDeadProcessId, 0 }));
}

TEST_CASE("SharedMemory lock recovery: clearIf keeps the record of a recycled process id")
{
  static unsigned int counter = 0;
  const std::string name = "shm_owner_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  SegmentLockOwner owner(name);

  // A record whose id matches a process that is alive, but whose start ticks
  // do not: the recorded holder is an earlier process that had the id and is
  // gone - the id was recycled. Clearing by id alone would wipe that record.
  const ProcessIdentity gone{ .processId = currentProcessId(),
    .startTicks = processStartTicksOf(currentProcessId()) + 1 };
  owner.set(gone);
  const ProcessIdentity recycledId{ .processId = currentProcessId(),
    .startTicks = processStartTicksOf(currentProcessId()) };

  REQUIRE_FALSE(owner.clearIf(recycledId));
  REQUIRE(owner.identity().processId == gone.processId);
  REQUIRE(owner.identity().startTicks == gone.startTicks);

  // the record's own identity can still drop it
  REQUIRE(owner.clearIf(gone));
  REQUIRE(owner.pid() == 0);
  SegmentLockOwner::removeOf(name);
}

TEST_CASE("SharedMemory lock recovery: a dead holder whose id was recycled is reclaimed")
{
  SharedMemoryServiceFixture fixture;
  boost::interprocess::named_semaphore stuck(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(stuck.try_wait());

  // same id as this (living) process, different start ticks: the recorded
  // holder is an earlier process with that id, and that one is gone
  SegmentLockOwner owner(fixture.name);
  owner.set(
    ProcessIdentity{ .processId = currentProcessId(), .startTicks = processStartTicksOf(currentProcessId()) + 1 });

  REQUIRE(fixture.service->recover() == 1);

  boost::interprocess::named_semaphore fresh(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(fresh.try_wait());
  REQUIRE_FALSE(fresh.try_wait());
  fresh.post();
}

TEST_CASE("SharedMemory lock recovery: a live holder that takes over during the probe is left alone")
{
  SharedMemoryServiceFixture fixture;
  // a dead holder left its record behind and the lock is taken
  boost::interprocess::named_semaphore stuck(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(stuck.try_wait());
  SegmentLockOwner owner(fixture.name);
  owner.set(kDeadProcessId);

  // The recovery snapshots the dead record, then waits out the probe delay.
  // Within that window a live process takes the lock over and records itself -
  // reclaiming now would hand out a second, independent lock. The widened probe
  // delay turns that window into a deterministic test.
  std::thread liveHolder([&owner]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    owner.set(currentProcessId());
  });

  const bool reclaimed = shm::recoverOrphanedSegmentLock(fixture.name, fixture.path, owner, std::chrono::seconds(2));
  liveHolder.join();

  REQUIRE_FALSE(reclaimed);
  REQUIRE(owner.pid() == currentProcessId());
  // the semaphore was not replaced: a fresh open sees the still taken object
  // instead of a recreated, free one
  boost::interprocess::named_semaphore same(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE_FALSE(same.try_wait());
}

TEST_CASE("SharedMemory lock recovery: a holder that releases during the probe keeps its lock")
{
  SharedMemoryServiceFixture fixture;
  boost::interprocess::named_semaphore stuck(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(stuck.try_wait());
  SegmentLockOwner owner(fixture.name);
  owner.set(kDeadProcessId);

  // the successor of the dead holder finishes its work and releases the lock
  // while the recovery waits out the probe delay - the lock is not stuck, so
  // the semaphore must survive
  std::thread finishingHolder([&owner, &stuck]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    owner.clear();
    stuck.post();
  });

  const bool reclaimed = shm::recoverOrphanedSegmentLock(fixture.name, fixture.path, owner, std::chrono::seconds(2));
  finishingHolder.join();

  REQUIRE_FALSE(reclaimed);
  boost::interprocess::named_semaphore same(
    boost::interprocess::open_only, shm::remapSemaphoreNameOf(fixture.name).c_str());
  REQUIRE(same.try_wait());
  same.post();
}

TEST_CASE("SharedMemoryService refuses a segment written with an older layout")
{
  // Default policy: nothing is discarded, the operator has to decide. The
  // exception names the file so the decision can be made deliberately.
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_service_cfg" };
  const std::filesystem::path path = uniqueSegmentPath();
  const std::string name = uniqueSegmentName();

  {
    boost::interprocess::managed_mapped_file old(boost::interprocess::create_only, path.c_str(), kInitialSize);
    old.construct<int>("stale_object")(42);
  }

  auto segment = std::make_shared<SharedMemorySegment>(path, name, kInitialSize);
  auto config = std::make_shared<Configuration>(
    std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
  config->setEntries(
    { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
  auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
  auto scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-service-test"));

  REQUIRE_THROWS_AS(
    std::make_shared<SharedMemoryService>(segmentManager, scheduler, std::make_shared<ProcessName>("shm-service-test")),
    std::runtime_error);

  // the file is untouched: the operator still has it for a migration
  {
    boost::interprocess::managed_mapped_file current(boost::interprocess::open_only, path.c_str());
    REQUIRE(current.find<int>("stale_object").first != nullptr);
  }

  scheduler.reset();
  std::filesystem::remove(path);
  boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(name).c_str());
  SegmentLockOwner::removeOf(name);
}

TEST_CASE("SharedMemoryService recreates a segment when the policy allows it")
{
  // LINTEL_SHM_RECREATE=1 is the explicit opt-in for a developer machine; the
  // contents are discarded, not migrated.
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_service_cfg" };
  ScopedEnvironmentVariable recreate{ "LINTEL_SHM_RECREATE", "1" };
  const std::filesystem::path path = uniqueSegmentPath();
  const std::string name = uniqueSegmentName();

  {
    // exactly what an older build leaves behind: named objects, no marker
    boost::interprocess::managed_mapped_file old(boost::interprocess::create_only, path.c_str(), kInitialSize);
    old.construct<int>("stale_object")(42);
    REQUIRE(old.find<int>("stale_object").first != nullptr);
  }

  auto segment = std::make_shared<SharedMemorySegment>(path, name, kInitialSize);
  auto config = std::make_shared<Configuration>(
    std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
  config->setEntries(
    { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
  auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
  auto scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-service-test"));

  auto service =
    std::make_shared<SharedMemoryService>(segmentManager, scheduler, std::make_shared<ProcessName>("shm-service-test"));

  {
    boost::interprocess::managed_mapped_file current(boost::interprocess::open_only, path.c_str());
    REQUIRE(current.find<int>("stale_object").first == nullptr);
    const auto marker = current.find<std::uint64_t>("lintel_segment_layout");
    REQUIRE(marker.first != nullptr);
    REQUIRE(*marker.first == 3);
    current.construct<int>("survivor")(7);
  }

  // a segment that already carries the current marker is kept, not recreated
  auto reopened =
    std::make_shared<SharedMemoryService>(segmentManager, scheduler, std::make_shared<ProcessName>("shm-service-test"));
  {
    boost::interprocess::managed_mapped_file current(boost::interprocess::open_only, path.c_str());
    REQUIRE(current.find<int>("survivor").first != nullptr);
  }

  reopened.reset();
  service.reset();
  scheduler.reset();
  std::filesystem::remove(path);
  boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(name).c_str());
  SegmentLockOwner::removeOf(name);
}

TEST_CASE("SharedMemoryService lets a registered migrator decide what happens")
{
  // The migrator runs while the stale segment is still on disk, so it can read
  // the payloads and carry them somewhere durable before anything is removed.
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_service_cfg" };
  const std::filesystem::path path = uniqueSegmentPath();
  const std::string name = uniqueSegmentName();

  {
    boost::interprocess::managed_mapped_file old(boost::interprocess::create_only, path.c_str(), kInitialSize);
    old.construct<int>("stale_object")(42);
  }

  struct Seen
  {
    std::filesystem::path path;
    std::uint64_t found = 0;
    std::uint64_t current = 0;
    int value = 0;
  };
  Seen seen;
  shm::SegmentLayoutPolicy::setMigrator(
    [&seen](const std::filesystem::path &stale, std::uint64_t found, std::uint64_t current) {
      seen.path = stale;
      seen.found = found;
      seen.current = current;
      // the old segment is still intact, so its payload can be read
      boost::interprocess::managed_mapped_file old(boost::interprocess::open_only, stale.c_str());
      const auto object = old.find<int>("stale_object");
      if (object.first == nullptr) { return false; }
      seen.value = *object.first;
      return true;// "carried over, you may start empty"
    });

  auto segment = std::make_shared<SharedMemorySegment>(path, name, kInitialSize);
  auto config = std::make_shared<Configuration>(
    std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
  config->setEntries(
    { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
  auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
  auto scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-service-test"));

  auto service =
    std::make_shared<SharedMemoryService>(segmentManager, scheduler, std::make_shared<ProcessName>("shm-service-test"));

  REQUIRE(seen.path == path);
  REQUIRE(seen.found == 0);// a segment from before the marker existed
  REQUIRE(seen.current == 3);
  REQUIRE(seen.value == 42);

  {
    boost::interprocess::managed_mapped_file current(boost::interprocess::open_only, path.c_str());
    REQUIRE(current.find<int>("stale_object").first == nullptr);
  }

  service.reset();
  scheduler.reset();
  std::filesystem::remove(path);
  boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(name).c_str());
  SegmentLockOwner::removeOf(name);
  shm::SegmentLayoutPolicy::reset();
}

TEST_CASE("SharedMemoryService aborts when the migrator refuses")
{
  ScopedEnvironmentVariable configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_service_cfg" };
  const std::filesystem::path path = uniqueSegmentPath();
  const std::string name = uniqueSegmentName();

  {
    boost::interprocess::managed_mapped_file old(boost::interprocess::create_only, path.c_str(), kInitialSize);
    old.construct<int>("stale_object")(42);
  }

  shm::SegmentLayoutPolicy::setMigrator(
    [](const std::filesystem::path & /*stale*/, std::uint64_t /*found*/, std::uint64_t /*current*/) { return false; });

  auto segment = std::make_shared<SharedMemorySegment>(path, name, kInitialSize);
  auto config = std::make_shared<Configuration>(
    std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
  config->setEntries(
    { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
  auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
  auto scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-service-test"));

  REQUIRE_THROWS_WITH(
    std::make_shared<SharedMemoryService>(segmentManager, scheduler, std::make_shared<ProcessName>("shm-service-test")),
    Catch::Matchers::ContainsSubstring("migrator refused"));

  {
    // nothing was removed: the operator can still migrate offline
    boost::interprocess::managed_mapped_file current(boost::interprocess::open_only, path.c_str());
    REQUIRE(current.find<int>("stale_object").first != nullptr);
  }

  scheduler.reset();
  std::filesystem::remove(path);
  boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(name).c_str());
  SegmentLockOwner::removeOf(name);
  shm::SegmentLayoutPolicy::reset();
}

}// namespace lintel
