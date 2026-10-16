#include <boost/interprocess/sync/named_semaphore.hpp>

#include <unistd.h>

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
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/SharedMemorySegmentComponent.h"
#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"
#include "lintel/features/base/repositories/SharedMemoryRepository.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryService.h"
#include "lintel/features/base/services/ShmConstructors.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  constexpr std::size_t kRepositorySegmentSize = 1u << 20;

  std::filesystem::path uniqueRepositoryShmPath()
  {
    static unsigned int counter = 0;
    return std::filesystem::temp_directory_path()
           / ("shm_repository_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
  }

  std::string uniqueRepositorySegmentName()
  {
    static unsigned int counter = 0;
    return "shm_repository_segment_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  /** Minimal JSON DAO so the Array/Object templates can be instantiated with a POD element. */
  class IntDao : public JsonSerializable
  {
  public:
    int m_value;
    explicit IntDao(int value) : m_value(value) {}

    void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override { writer->Int(m_value); }

    bool deserialize(const rapidjson::Value & /*obj*/) override { return false; }
  };

  // The repository templates leave onMigrate() pure; concrete subclasses make them instantiable.
  using ArrayBase = SharedMemoryArrayRepository<int, IntDao, 4>;
  class TestArrayRepository : public ArrayBase
  {
  public:
    using ArrayBase::ArrayBase;
    void onMigrate(int32_t /*currentActiveVersion*/) override {}
  };

  using VectorBase = SharedMemoryVectorRepository<shm::String, shm::String>;
  class TestVectorRepository : public VectorBase
  {
  public:
    using VectorBase::VectorBase;
    void onMigrate(int32_t /*currentActiveVersion*/) override {}
  };

  using MapBase = SharedMemoryMapRepository<shm::String, shm::String, shm::String, shm::String>;
  class TestMapRepository : public MapBase
  {
  public:
    using MapBase::MapBase;
    void onMigrate(int32_t /*currentActiveVersion*/) override {}
  };

  using ObjectBase = SharedMemoryObjectRepository<int, IntDao>;
  class TestObjectRepository : public ObjectBase
  {
  public:
    using ObjectBase::ObjectBase;
    void onMigrate(int32_t /*currentActiveVersion*/) override {}
  };

  /** Wires a SharedMemoryService over one real segment so the repository templates can be instantiated. */
  struct ShmRepositoryFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_repository_cfg" };
    std::filesystem::path path;
    std::string segmentName;
    std::shared_ptr<SharedMemorySegment> segment;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemoryService> service;

    ShmRepositoryFixture() : path(uniqueRepositoryShmPath()), segmentName(uniqueRepositorySegmentName())
    {
      segment = std::make_shared<SharedMemorySegment>(path, segmentName, kRepositorySegmentSize);
      auto config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
      auto segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
      scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-repository-test"));
      service = std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("shm-repository-test"));
    }

    ~ShmRepositoryFixture()
    {
      service.reset();
      scheduler.reset();
      std::filesystem::remove(path);
      boost::interprocess::named_semaphore::remove((segmentName + "_remap_sem").c_str());
    }
  };

  std::string repositoryToJson(const JsonSerializable &repository)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    repository.serialize(&writer);
    return std::string(buffer.GetString());
  }

}// namespace

TEST_CASE("SharedMemoryArrayRepository serializes its elements and reports its type")
{
  ShmRepositoryFixture fixture;
  TestArrayRepository repository(fixture.service, fixture.segment, 0, "array-uuid");

  REQUIRE(repository.getType() == SharedMemoryType::Array);
  REQUIRE(repository.getTypeName() == "Array");

  REQUIRE(repositoryToJson(repository).find("SharedMemoryArrayRepository") != std::string::npos);

  // a grow invalidates the cached pointer and forces a re-fetch
  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(repositoryToJson(repository).find("SharedMemoryArrayRepository") != std::string::npos);

  REQUIRE_THROWS_AS(repository.deserialize(rapidjson::Value()), std::runtime_error);
}

TEST_CASE("SharedMemoryVectorRepository serializes its elements and reports its type")
{
  ShmRepositoryFixture fixture;
  TestVectorRepository repository(fixture.service, fixture.segment, 0, "vector-uuid");

  REQUIRE(repository.getType() == SharedMemoryType::Vector);
  REQUIRE(repository.getTypeName() == "Vector");

  auto &vector =
    shm::constructVector<shm::String>(*fixture.service, fixture.segment, repository.getSharedMemoryRepository());
  vector.push_back(shm::constructString(*fixture.service, fixture.segment, "entry"));

  REQUIRE(repositoryToJson(repository).find("entry") != std::string::npos);

  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(repositoryToJson(repository).find("entry") != std::string::npos);

  REQUIRE_THROWS_AS(repository.deserialize(rapidjson::Value()), std::runtime_error);
}

TEST_CASE("SharedMemoryMapRepository serializes its entries and reports its type")
{
  ShmRepositoryFixture fixture;
  TestMapRepository repository(fixture.service, fixture.segment, 0, "map-uuid");

  REQUIRE(repository.getType() == SharedMemoryType::Map);
  REQUIRE(repository.getTypeName() == "Map");

  auto &map = shm::constructMap<shm::String, shm::String>(
    *fixture.service, fixture.segment, repository.getSharedMemoryRepository());
  map.insert({ shm::constructString(*fixture.service, fixture.segment, "key"),
    shm::constructString(*fixture.service, fixture.segment, "value") });

  const std::string json = repositoryToJson(repository);
  REQUIRE(json.find("SharedMemoryMapRepository") != std::string::npos);
  REQUIRE(json.find("key") != std::string::npos);
  REQUIRE(json.find("value") != std::string::npos);

  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(repositoryToJson(repository).find("value") != std::string::npos);

  REQUIRE_THROWS_AS(repository.deserialize(rapidjson::Value()), std::runtime_error);
}

TEST_CASE("SharedMemoryObjectRepository serializes its object and reports its type")
{
  ShmRepositoryFixture fixture;
  TestObjectRepository repository(fixture.service, fixture.segment, 0, "object-uuid");

  REQUIRE(repository.getType() == SharedMemoryType::Object);
  REQUIRE(repository.getTypeName() == "Object");

  REQUIRE(repositoryToJson(repository).find("SharedMemoryObjectRepository") != std::string::npos);

  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(repositoryToJson(repository).find("SharedMemoryObjectRepository") != std::string::npos);

  REQUIRE_THROWS_AS(repository.deserialize(rapidjson::Value()), std::runtime_error);
}

TEST_CASE("SharedMemoryRepository exposes its constructor metadata")
{
  ShmRepositoryFixture fixture;
  TestObjectRepository repository(fixture.service, fixture.segment, 7, "metadata-uuid");

  REQUIRE(repository.getSharedMemorySegment() == fixture.segment);
  REQUIRE(repository.getCodeVersion() == 7);
  REQUIRE(repository.getUuid() == "metadata-uuid");
  REQUIRE(repository.getSizeOfData() == sizeof(int));
}

}// namespace lintel
