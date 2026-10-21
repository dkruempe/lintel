#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
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
#include "lintel/features/base/services/SharedMemoryCleanupService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryService.h"
#include "lintel/features/base/services/ShmConstructors.h"

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  constexpr std::size_t kCleanupSegmentSize = 1u << 20;

  /** A process id that provably does not exist (above Linux's PID_MAX_LIMIT). */
  constexpr ProcessId kDeadProcessId = static_cast<ProcessId>(1) << 30;

  ProcessIdentity currentIdentity()
  {
    const ProcessId pid = currentProcessId();
    return ProcessIdentity{ .processId = pid, .startTicks = processStartTicksOf(pid) };
  }

  std::filesystem::path uniqueCleanupPath()
  {
    static unsigned int counter = 0;
    return std::filesystem::temp_directory_path()
           / ("shm_cleanup_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
  }

  std::string uniqueCleanupSegmentName()
  {
    static unsigned int counter = 0;
    return "shm_cln_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  struct CleanupFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_cleanup_cfg" };
    std::filesystem::path path;
    std::string segmentName;
    std::shared_ptr<SharedMemorySegment> segment;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemoryService> service;
    std::shared_ptr<SharedMemoryCleanupService> cleanup;

    CleanupFixture() : path(uniqueCleanupPath()), segmentName(uniqueCleanupSegmentName())
    {
      segment = std::make_shared<SharedMemorySegment>(path, segmentName, kCleanupSegmentSize);
      auto config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
      auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
      scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-cleanup-test"));
      service = std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("shm-cleanup-test"));
      cleanup = std::make_shared<SharedMemoryCleanupService>(scheduler, service);
    }

    ~CleanupFixture()
    {
      cleanup.reset();
      service.reset();
      scheduler.reset();
      std::filesystem::remove(path);
      boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(segmentName).c_str());
      SegmentLockOwner::removeOf(segmentName);
    }

    int &createObject(const std::string &name, int value = 42)
    {
      int &object = shm::constructObject<int>(*service, segment, name);
      object = value;
      return object;
    }

    [[nodiscard]] bool exists(const std::string &name) const
    {
      return shm::findNamedObject(*service, segment, name) != nullptr;
    }
  };

}// namespace

TEST_CASE("SharedMemoryCleanupService: an empty registry has no orphans")
{
  CleanupFixture fixture;

  const SharedMemoryCleanupReport scan = fixture.cleanup->scan();
  REQUIRE(scan.dryRun);
  REQUIRE(scan.empty());
  REQUIRE(scan.orphanCount() == 0);
  REQUIRE(scan.removed.empty());
  REQUIRE(scan.scanned == 0);

  const SharedMemoryCleanupReport applied = fixture.cleanup->apply();
  REQUIRE_FALSE(applied.dryRun);
  REQUIRE(applied.empty());
  REQUIRE(applied.removed.empty());
}

TEST_CASE("SharedMemoryCleanupService: dry-run reports an orphan without removing it")
{
  CleanupFixture fixture;
  fixture.createObject("orphan");
  REQUIRE(fixture.exists("orphan"));

  const bool tracked = fixture.cleanup->trackObject<int>(
    fixture.service, fixture.segment, "orphan", ProcessIdentity{ .processId = kDeadProcessId, .startTicks = 0 });
  REQUIRE(tracked);
  REQUIRE(fixture.cleanup->size() == 1);

  const SharedMemoryCleanupReport scan = fixture.cleanup->scan();

  REQUIRE(scan.dryRun);
  REQUIRE(scan.orphanCount() == 1);
  REQUIRE(scan.orphans.front().objectName == "orphan");
  REQUIRE(scan.orphans.front().segmentName == fixture.segmentName);
  REQUIRE(scan.orphans.front().owner.processId == kDeadProcessId);
  REQUIRE(scan.orphans.front().reason.find("gone") != std::string::npos);
  REQUIRE(scan.removed.empty());
  REQUIRE(fixture.exists("orphan"));
  REQUIRE(fixture.cleanup->size() == 1);
}

TEST_CASE("SharedMemoryCleanupService: apply removes an orphaned object")
{
  CleanupFixture fixture;
  fixture.createObject("orphan");
  fixture.cleanup->trackObject<int>(
    fixture.service, fixture.segment, "orphan", ProcessIdentity{ .processId = kDeadProcessId, .startTicks = 0 });

  const SharedMemoryCleanupReport applied = fixture.cleanup->apply();

  REQUIRE_FALSE(applied.dryRun);
  REQUIRE(applied.orphanCount() == 1);
  REQUIRE(applied.removed.size() == 1);
  REQUIRE(applied.removed.front().objectName == "orphan");
  REQUIRE(applied.failed == 0);
  REQUIRE_FALSE(fixture.exists("orphan"));
  REQUIRE(fixture.cleanup->size() == 0);

  const SharedMemoryCleanupStats stats = fixture.cleanup->stats();
  REQUIRE(stats.applies == 1);
  REQUIRE(stats.orphans == 1);
  REQUIRE(stats.removed == 1);
  REQUIRE(stats.failures == 0);
}

TEST_CASE("SharedMemoryCleanupService: a live owner is never touched")
{
  CleanupFixture fixture;
  fixture.createObject("live");
  fixture.cleanup->trackObject<int>(fixture.service, fixture.segment, "live", currentIdentity());

  const SharedMemoryCleanupReport scan = fixture.cleanup->scan();
  REQUIRE(scan.empty());

  const SharedMemoryCleanupReport applied = fixture.cleanup->apply();
  REQUIRE(applied.empty());
  REQUIRE(applied.removed.empty());
  REQUIRE(fixture.exists("live"));
  REQUIRE(fixture.cleanup->size() == 1);
}

TEST_CASE("SharedMemoryCleanupService: a stale heartbeat of a live owner is not an orphan")
{
  CleanupFixture fixture;
  fixture.createObject("slow");
  fixture.cleanup->trackObject<int>(fixture.service,
    fixture.segment,
    "slow",
    currentIdentity(),
    std::chrono::system_clock::now() - std::chrono::hours(24));

  const SharedMemoryCleanupReport report = fixture.cleanup->apply();
  REQUIRE(report.empty());
  REQUIRE(fixture.exists("slow"));
}

TEST_CASE("SharedMemoryCleanupService: repeated runs are idempotent")
{
  CleanupFixture fixture;
  fixture.createObject("orphan");
  fixture.cleanup->trackObject<int>(
    fixture.service, fixture.segment, "orphan", ProcessIdentity{ .processId = kDeadProcessId, .startTicks = 0 });

  const SharedMemoryCleanupReport first = fixture.cleanup->apply();
  REQUIRE(first.removed.size() == 1);

  const SharedMemoryCleanupReport second = fixture.cleanup->apply();
  REQUIRE(second.empty());
  REQUIRE(second.removed.empty());
  REQUIRE(second.failed == 0);
  REQUIRE(fixture.cleanup->size() == 0);
  REQUIRE_FALSE(fixture.exists("orphan"));
}

TEST_CASE("SharedMemoryCleanupService: a failed remover keeps the entry for a retry")
{
  CleanupFixture fixture;
  fixture.createObject("stuck");
  int attempts = 0;
  fixture.cleanup->trackObject(
    fixture.segmentName, "stuck", ProcessIdentity{ .processId = kDeadProcessId, .startTicks = 0 }, [&attempts]() {
      ++attempts;
      return false;
    });

  const SharedMemoryCleanupReport applied = fixture.cleanup->apply();
  REQUIRE(applied.orphanCount() == 1);
  REQUIRE(applied.removed.empty());
  REQUIRE(applied.failed == 1);
  REQUIRE(attempts == 1);
  REQUIRE(fixture.cleanup->size() == 1);
  REQUIRE(fixture.exists("stuck"));

  // A later run retries the same entry.
  fixture.cleanup->apply();
  REQUIRE(attempts == 2);
}

TEST_CASE("SharedMemoryCleanupService: touch and untrack update the registry")
{
  CleanupFixture fixture;
  fixture.createObject("object");
  const ProcessIdentity owner{ .processId = kDeadProcessId, .startTicks = 0 };
  REQUIRE(fixture.cleanup->trackObject<int>(fixture.service, fixture.segment, "object", owner));

  REQUIRE(fixture.cleanup->touchObject(fixture.segmentName, "object"));
  REQUIRE_FALSE(fixture.cleanup->touchObject(fixture.segmentName, "missing"));
  REQUIRE(fixture.cleanup->untrackObject(fixture.segmentName, "object"));
  REQUIRE_FALSE(fixture.cleanup->untrackObject(fixture.segmentName, "object"));
  REQUIRE(fixture.cleanup->size() == 0);
  REQUIRE(fixture.exists("object"));
}

TEST_CASE("SharedMemoryCleanupService: the automatic interval is configurable")
{
  CleanupFixture fixture;
  REQUIRE(fixture.cleanup->interval() == std::chrono::seconds(60));
  REQUIRE(fixture.cleanup->applyScheduled());

  fixture.cleanup->setInterval(std::chrono::seconds(5));
  fixture.cleanup->setApplyScheduled(false);
  REQUIRE(fixture.cleanup->interval() == std::chrono::seconds(5));
  REQUIRE_FALSE(fixture.cleanup->applyScheduled());
}

}// namespace lintel
