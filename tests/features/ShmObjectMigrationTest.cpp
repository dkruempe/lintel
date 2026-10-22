#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/shm/SegmentLayoutPolicy.h"
#include "lintel/core/shm/SharedMemoryRepository.h"
#include "lintel/core/shm/ShmObjectHeader.h"
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

  constexpr std::size_t kMigrationSegmentSize = 1u << 20;

  std::filesystem::path uniqueMigrationPath()
  {
    static unsigned int counter = 0;
    return std::filesystem::temp_directory_path()
           / ("shm_migration_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
  }

  // macOS caps POSIX semaphore names at 31 characters; the service appends
  // "_remap_sem" (10), so the segment name stays short.
  std::string uniqueMigrationSegmentName()
  {
    static unsigned int counter = 0;
    return "shm_mig_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  /** Payload whose first version only used the `value` field. */
  struct VersionedPayload
  {
    int value = 0;
    int reserved = 0;
  };

  /** One real segment that can be closed and reopened, so a payload written by
   *  an older "build" (version 1) can be read by the current one (version 2). */
  struct MigrationFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_migration_cfg" };
    std::filesystem::path path;
    std::string segmentName;
    std::shared_ptr<SharedMemorySegment> segment;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemorySegmentManager> segmentManager;
    std::shared_ptr<SharedMemoryService> service;

    MigrationFixture() : path(uniqueMigrationPath()), segmentName(uniqueMigrationSegmentName())
    {
      std::filesystem::remove(path);
      segment = std::make_shared<SharedMemorySegment>(path, segmentName, kMigrationSegmentSize);
      auto config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
      segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
      scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-migration-test"));
      service = makeService();
    }

    ~MigrationFixture()
    {
      service.reset();
      scheduler.reset();
      SegmentLockOwner::removeOf(segmentName);
      std::filesystem::remove(path);
      boost::interprocess::named_semaphore::remove(shm::remapSemaphoreNameOf(segmentName).c_str());
      shm::ShmPayloadMigrations::reset();
      shm::SegmentLayoutPolicy::reset();
    }

    [[nodiscard]] std::shared_ptr<SharedMemoryService> makeService() const
    {
      return std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("shm-migration-test"));
    }

    /** Close and reopen the segment, like a process restart. */
    void restart()
    {
      service.reset();
      service = makeService();
    }

    /** Rewrite the segment marker to a value older than the current layout. */
    void stampLayoutVersion(std::uint64_t version) const
    {
      const std::string filePath = path.string();
      boost::interprocess::managed_mapped_file mapping(boost::interprocess::open_only, filePath.c_str());
      auto found = mapping.find<std::uint64_t>("lintel_segment_layout");
      REQUIRE(found.first != nullptr);
      *found.first = version;
    }
  };

  [[nodiscard]] shm::ShmObjectOptions optionsOf(std::uint16_t version,
    shm::ShmPersistence persistence = shm::ShmPersistence::Persistent)
  {
    return shm::ShmObjectOptions{ .payloadVersion = version, .persistence = persistence };
  }

}// namespace

TEST_CASE("ShmObjectHeader has a stable on-disk layout")
{
  REQUIRE(sizeof(shm::ShmObjectHeader) == 64);
  REQUIRE(alignof(shm::ShmObjectHeader) == 16);
  REQUIRE(shm::ShmObjectHeader::kMagic == 0x4c53484fU);
  REQUIRE(shm::ShmObjectHeader{}.isCompatible());
  REQUIRE_FALSE(shm::ShmObjectHeader{}.isReady());
}

TEST_CASE("a typed repository migrates its payload through a registered chain")
{
  MigrationFixture fixture;
  const shm::ShmObjectOptions versionOne = optionsOf(1);
  {
    shm::SharedMemoryRepository<VersionedPayload> repository(
      fixture.service, fixture.segment, "versioned", versionOne, VersionedPayload{ 42, 0 });
    repository.get().value = 42;
    REQUIRE(repository.getPayloadVersion() == 1);
  }
  fixture.restart();

  shm::registerPayloadMigration<VersionedPayload>(
    { .fromVersion = 1, .toVersion = 2, .apply = [](const void *source, std::size_t /*sourceBytes*/, void *target) {
       const auto *oldValue = static_cast<const VersionedPayload *>(source);
       ::new (target) VersionedPayload{ oldValue->value * 2, 7 };
     } });

  shm::SharedMemoryRepository<VersionedPayload> migrated(
    fixture.service, fixture.segment, "versioned", optionsOf(2), VersionedPayload{});
  REQUIRE(migrated.get().value == 84);
  REQUIRE(migrated.get().reserved == 7);
  REQUIRE(migrated.getPayloadVersion() == 2);
}

TEST_CASE("a persistent payload without a migration aborts the start")
{
  MigrationFixture fixture;
  {
    shm::SharedMemoryRepository<VersionedPayload> repository(
      fixture.service, fixture.segment, "persistent", optionsOf(1), VersionedPayload{ 42, 0 });
    repository.get().value = 42;
  }
  fixture.restart();
  shm::ShmPayloadMigrations::reset();

  REQUIRE_THROWS_AS((shm::SharedMemoryRepository<VersionedPayload>(
                      fixture.service, fixture.segment, "persistent", optionsOf(2), VersionedPayload{})),
    std::runtime_error);
}

TEST_CASE("a volatile payload is recreated when its version is unknown")
{
  MigrationFixture fixture;
  {
    shm::SharedMemoryRepository<VersionedPayload> repository(fixture.service,
      fixture.segment,
      "volatile",
      optionsOf(1, shm::ShmPersistence::Volatile),
      VersionedPayload{ 42, 0 });
    repository.get().value = 42;
  }
  fixture.restart();
  shm::ShmPayloadMigrations::reset();

  shm::SharedMemoryRepository<VersionedPayload> recreated(fixture.service,
    fixture.segment,
    "volatile",
    optionsOf(2, shm::ShmPersistence::Volatile),
    VersionedPayload{ 7, 0 });
  REQUIRE(recreated.get().value == 7);
  REQUIRE(recreated.getPayloadVersion() == 2);
  REQUIRE(recreated.getPersistence() == shm::ShmPersistence::Volatile);
}

TEST_CASE("a derived payload is recreated when its version is unknown")
{
  MigrationFixture fixture;
  {
    shm::SharedMemoryRepository<VersionedPayload> repository(fixture.service,
      fixture.segment,
      "derived",
      optionsOf(1, shm::ShmPersistence::Derived),
      VersionedPayload{ 42, 0 });
    repository.get().value = 42;
  }
  fixture.restart();
  shm::ShmPayloadMigrations::reset();

  shm::SharedMemoryRepository<VersionedPayload> recreated(
    fixture.service, fixture.segment, "derived", optionsOf(2, shm::ShmPersistence::Derived), VersionedPayload{ 11, 0 });
  REQUIRE(recreated.get().value == 11);
  REQUIRE(recreated.getPersistence() == shm::ShmPersistence::Derived);
}

TEST_CASE("a segment with only volatile objects is recreated automatically")
{
  MigrationFixture fixture;
  {
    shm::SharedMemoryRepository<VersionedPayload> repository(fixture.service,
      fixture.segment,
      "auto-volatile",
      optionsOf(1, shm::ShmPersistence::Volatile),
      VersionedPayload{ 42, 0 });
    repository.get().value = 42;
  }
  fixture.service.reset();
  fixture.stampLayoutVersion(2);

  REQUIRE_NOTHROW(fixture.service = fixture.makeService());
  const std::string filePath = fixture.path.string();
  boost::interprocess::managed_mapped_file current(boost::interprocess::open_only, filePath.c_str());
  REQUIRE(current.find<char>("auto-volatile").first == nullptr);
  auto marker = current.find<std::uint64_t>("lintel_segment_layout");
  REQUIRE(marker.first != nullptr);
  REQUIRE(*marker.first == 3);
}

TEST_CASE("a segment with a persistent object still refuses an automatic discard")
{
  MigrationFixture fixture;
  {
    shm::SharedMemoryRepository<VersionedPayload> repository(
      fixture.service, fixture.segment, "auto-persistent", optionsOf(1), VersionedPayload{ 42, 0 });
    repository.get().value = 42;
  }
  fixture.service.reset();
  fixture.stampLayoutVersion(2);

  REQUIRE_THROWS_AS((fixture.service = fixture.makeService()), std::runtime_error);
}

}// namespace lintel
