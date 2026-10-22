#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/shm/SharedMemoryRepository.h"
#include "lintel/core/shm/SharedMemoryRepositoryRegistry.h"
#include "lintel/core/shm/containers/ArrayShm.h"
#include "lintel/core/shm/containers/QueueShm.h"
#include "lintel/core/shm/containers/RingBufferShm.h"
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

  // Keep the segment name short: macOS caps POSIX semaphore names at 31
  // characters and SharedMemoryService appends "_remap_sem" (10) to it.
  std::string uniqueRepositorySegmentName()
  {
    static unsigned int counter = 0;
    return "shm_rep_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
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
    std::shared_ptr<SharedMemorySegmentManager> segmentManager;
    std::shared_ptr<SharedMemoryService> service;

    ShmRepositoryFixture() : path(uniqueRepositoryShmPath()), segmentName(uniqueRepositorySegmentName())
    {
      segment = std::make_shared<SharedMemorySegment>(path, segmentName, kRepositorySegmentSize);
      auto config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
      segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
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

    /** A second independent service over the same segment file - what a second
     *  process attaching to the segment gets: its own mapping, its own
     *  generation counter, its own handle bookkeeping. */
    [[nodiscard]] std::shared_ptr<SharedMemoryService> openSecondService() const
    {
      return std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("shm-repository-test-b"));
    }
  };

  /** Simple shared memory payload for the typed repository tests: two ints, no pointers. */
  struct CellPayload
  {
    int m_value = 0;
    int m_counter = 0;
  };

  /** Bump allocator over one externally owned buffer. The whole "segment" is
   *  therefore contiguous memory that can be copied to another address in one
   *  memcpy - the situation of a segment mapped at a different virtual address
   *  by another process. */
  class RegionAllocator : public ShmSegmentAccessor::Allocator
  {
  public:
    RegionAllocator(std::byte *base, std::size_t size) : m_base(base), m_size(size) {}

    void *allocate(std::size_t bytes) override
    {
      if (m_used > m_size) { return nullptr; }
      void *candidate = m_base + m_used;
      std::size_t space = m_size - m_used;
      // Cache line alignment, like the segment allocator gives the slot array.
      if (std::align(RingBufferShm<int>::CACHE_LINE, bytes, candidate, space) == nullptr) { return nullptr; }
      m_used = static_cast<std::size_t>(static_cast<std::byte *>(candidate) - m_base) + bytes;
      return candidate;
    }

    void deallocate(void * /*pointer*/) override {}

  private:
    std::byte *m_base;
    std::size_t m_size;
    std::size_t m_used = 0;
  };

  /** Bytes of one simulated segment mapping; cache line aligned so a relocated
   *  ring still holds its cache line aligned slots. */
  struct alignas(RingBufferShm<int>::CACHE_LINE) SegmentRegion
  {
    static constexpr std::size_t SIZE = 4096;
    std::byte bytes[SIZE]{};
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

TEST_CASE("shm::SharedMemoryRepository stores a single object per name")
{
  ShmRepositoryFixture fixture;
  shm::SharedMemoryRepository<CellPayload> repository(fixture.service, fixture.segment, "cell", CellPayload{ 41, 2 });

  REQUIRE(repository.getName() == "cell");
  REQUIRE(repository.getSegment() == fixture.segment);
  REQUIRE(repository.get().m_value == 41);
  REQUIRE(repository.get().m_counter == 2);

  repository.set(CellPayload{ 7, 3 });
  REQUIRE(repository.get().m_value == 7);

  repository.update([](CellPayload &payload) { payload.m_counter += 10; });
  REQUIRE(repository.get().m_counter == 13);

  // a second binding on the same name finds the object instead of
  // re-running the constructor arguments
  shm::SharedMemoryRepository<CellPayload> second(fixture.service, fixture.segment, "cell", CellPayload{ 99, 99 });
  REQUIRE(second.get().m_value == 7);
  REQUIRE(second.get().m_counter == 13);
}

TEST_CASE("shm::SharedMemoryRepository detects a segment remap through the generation")
{
  ShmRepositoryFixture fixture;
  shm::SharedMemoryRepository<CellPayload> repository(
    fixture.service, fixture.segment, "gen-cell", CellPayload{ 5, 0 });

  REQUIRE(repository.generation() == fixture.service->getGeneration());
  REQUIRE_FALSE(repository.isStale());

  fixture.service->growOf(fixture.segment, 1u << 20);

  REQUIRE(repository.isStale());
  REQUIRE(repository.get().m_value == 5);// re-resolved in the new mapping
  REQUIRE_FALSE(repository.isStale());
  REQUIRE(repository.generation() == fixture.service->getGeneration());
}

TEST_CASE("shm::SharedMemoryRepository is found again from a second service")
{
  ShmRepositoryFixture fixture;
  {
    shm::SharedMemoryRepository<CellPayload> repository(
      fixture.service, fixture.segment, "xproc-cell", CellPayload{ 21, 4 });
    repository.update([](CellPayload &payload) { ++payload.m_value; });
  }

  auto secondService = fixture.openSecondService();
  shm::SharedMemoryRepository<CellPayload> attached(secondService, fixture.segment, "xproc-cell", CellPayload{});
  REQUIRE(attached.get().m_value == 22);
  REQUIRE(attached.get().m_counter == 4);
}

TEST_CASE("shm::SharedMemoryRepository rejects invalid construction")
{
  ShmRepositoryFixture fixture;

  REQUIRE_THROWS_AS(
    (shm::SharedMemoryRepository<CellPayload>(nullptr, fixture.segment, "bad", CellPayload{})), std::invalid_argument);
  REQUIRE_THROWS_AS((shm::SharedMemoryRepository<CellPayload>(fixture.service, fixture.segment, "", CellPayload{})),
    std::invalid_argument);

  // same name, different payload layout: the already existing cell must not
  // be reinterpreted with the new size
  shm::SharedMemoryRepository<CellPayload> cell(fixture.service, fixture.segment, "layout-cell", CellPayload{});
  REQUIRE_THROWS_AS(
    (shm::SharedMemoryRepository<int>(fixture.service, fixture.segment, "layout-cell", 1)), std::invalid_argument);
  static_cast<void>(cell);
}

TEST_CASE("ArrayShm in shared memory stores elements and checks bounds")
{
  ShmRepositoryFixture fixture;
  using IntArray = ArrayShm<int, 4>;
  shm::SharedMemoryRepository<IntArray> repository(fixture.service, fixture.segment, "array-cell");

  REQUIRE(repository.get().size() == 4);

  repository.get().set(0, 10);
  repository.get().at(1) = 20;
  repository.get()[2] = 30;
  REQUIRE(repository.get().at(0) == 10);
  REQUIRE(repository.get()[1] == 20);
  REQUIRE(repository.get().at(2) == 30);

  REQUIRE_THROWS_AS(repository.get().at(4), std::out_of_range);
  REQUIRE(repository.get().get(2) == 30);
  REQUIRE_THROWS_AS(repository.get().get(4), std::out_of_range);
  REQUIRE_THROWS_AS(repository.get().set(4, 1), std::out_of_range);

  repository.get().fill(5);
  REQUIRE(repository.get().at(3) == 5);

  // survives a remap like every shared memory object
  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(repository.get().at(3) == 5);
}

TEST_CASE("RingBufferShm in shared memory pushes and pops in order")
{
  ShmRepositoryFixture fixture;
  const ShmSegmentAccessor accessor = fixture.service->getSegmentAccessor(fixture.segment);
  shm::SharedMemoryRepository<RingBufferShm<int>> repository(
    fixture.service, fixture.segment, "ring-cell", std::size_t(4), accessor);
  auto &ring = repository.get();

  REQUIRE(ring.capacity() == 4);
  REQUIRE(ring.empty());
  REQUIRE(ring.isLockFree());

  REQUIRE(ring.push(1));
  REQUIRE(ring.push(2));
  REQUIRE(ring.push(3));
  REQUIRE(ring.push(4));
  REQUIRE(ring.full());
  REQUIRE_FALSE(ring.push(5));// full

  int value = 0;
  REQUIRE(ring.pop(value));
  REQUIRE(value == 1);
  REQUIRE(ring.pop(value));
  REQUIRE(value == 2);
  REQUIRE(ring.pop(value));
  REQUIRE(value == 3);
  REQUIRE(ring.pop(value));
  REQUIRE(value == 4);
  REQUIRE(ring.empty());
  REQUIRE_FALSE(ring.pop(value));// empty

  // wrap-around: more push/pop cycles than capacity
  for (int i = 0; i < 10; ++i) {
    REQUIRE(ring.push(i));
    REQUIRE(ring.pop(value));
    REQUIRE(value == i);
  }

  auto popped = ring.tryPop();
  REQUIRE_FALSE(popped.has_value());
}

TEST_CASE("RingBufferShm addresses its slots relative to the ring object")
{
  using Ring = RingBufferShm<int>;

  // Emulate the second mapping of a segment: ring and slot array live in one
  // contiguous region, that region is copied to another address and the
  // original is wiped. An address cached inside the ring would keep pointing
  // at the wiped region; the offset travels with the copy.
  auto firstRegion = std::make_unique<SegmentRegion>();
  RegionAllocator firstAllocator(firstRegion->bytes, SegmentRegion::SIZE);
  auto *ring = static_cast<Ring *>(firstAllocator.allocate(sizeof(Ring)));
  REQUIRE(ring != nullptr);
  ::new (static_cast<void *>(ring)) Ring(4, ShmSegmentAccessor(firstAllocator));
  REQUIRE(ring->push(7));
  REQUIRE(ring->push(8));

  const std::ptrdiff_t ringOffset = reinterpret_cast<std::byte *>(ring) - firstRegion->bytes;
  auto secondRegion = std::make_unique<SegmentRegion>();
  std::memcpy(secondRegion->bytes, firstRegion->bytes, SegmentRegion::SIZE);

  // The original mapping is gone: only the relocated bytes are still valid.
  std::fill_n(firstRegion->bytes, SegmentRegion::SIZE, std::byte{ 0 });

  auto *relocated = std::launder(reinterpret_cast<Ring *>(secondRegion->bytes + ringOffset));
  int value = 0;
  REQUIRE(relocated->pop(value));
  REQUIRE(value == 7);
  REQUIRE(relocated->push(9));
  REQUIRE(relocated->pop(value));
  REQUIRE(value == 8);
  REQUIRE(relocated->pop(value));
  REQUIRE(value == 9);
  REQUIRE(relocated->empty());
}

TEST_CASE("QueueShm in shared memory is FIFO")
{
  ShmRepositoryFixture fixture;
  const ShmSegmentAccessor accessor = fixture.service->getSegmentAccessor(fixture.segment);
  shm::SharedMemoryRepository<QueueShm<int>> repository(
    fixture.service, fixture.segment, "queue-cell", std::size_t(3), accessor);
  auto &queue = repository.get();

  REQUIRE(queue.capacity() == 3);
  REQUIRE(queue.empty());

  REQUIRE(queue.push(10));
  REQUIRE(queue.push(20));
  REQUIRE(queue.push(30));
  REQUIRE(queue.full());
  REQUIRE_FALSE(queue.push(40));

  int value = 0;
  REQUIRE(queue.pop(value));
  REQUIRE(value == 10);// oldest first
  REQUIRE(queue.push(40));
  REQUIRE(queue.pop(value));
  REQUIRE(value == 20);
  REQUIRE(queue.pop(value));
  REQUIRE(value == 30);
  REQUIRE(queue.pop(value));
  REQUIRE(value == 40);
  REQUIRE(queue.empty());

  auto popped = queue.tryPop();
  REQUIRE_FALSE(popped.has_value());
}

TEST_CASE("SharedMemoryRepositoryRegistry keeps typed repositories")
{
  ShmRepositoryFixture fixture;
  auto &registry = SharedMemoryRepositoryRegistry::instance();
  auto repository = std::make_shared<shm::SharedMemoryRepository<CellPayload>>(
    fixture.service, fixture.segment, "registry-cell", CellPayload{ 3, 1 });

  const std::size_t sizeBefore = registry.size();
  registry.registerRepository("test-registry-cell", repository);
  REQUIRE(registry.size() == sizeBefore + 1);
  REQUIRE(registry.contains("test-registry-cell"));

  auto found = registry.getAs<shm::SharedMemoryRepository<CellPayload>>("test-registry-cell");
  REQUIRE(found == repository);
  REQUIRE(found->get().m_value == 3);

  REQUIRE(registry.getAs<shm::SharedMemoryRepository<CellPayload>>("missing-name") == nullptr);
  REQUIRE_THROWS_AS(registry.getAs<int>("test-registry-cell"), std::invalid_argument);

  REQUIRE(registry.remove("test-registry-cell"));
  REQUIRE_FALSE(registry.contains("test-registry-cell"));
  REQUIRE_FALSE(registry.remove("test-registry-cell"));
}

TEST_CASE("shm::SharedMemoryRepository::of reuses one instance per segment and name")
{
  ShmRepositoryFixture fixture;
  auto &registry = SharedMemoryRepositoryRegistry::instance();
  const std::string cellKey = fixture.segment->getName() + ":of-cell";
  const std::string otherKey = fixture.segment->getName() + ":of-other";
  REQUIRE_FALSE(registry.contains(cellKey));

  auto first =
    shm::SharedMemoryRepository<CellPayload>::of(fixture.service, fixture.segment, "of-cell", CellPayload{ 5, 6 });
  REQUIRE(registry.contains(cellKey));
  REQUIRE(first->get().m_value == 5);

  // same segment and name: the registered instance comes back, the constructor
  // arguments of the second call are not used again
  auto second =
    shm::SharedMemoryRepository<CellPayload>::of(fixture.service, fixture.segment, "of-cell", CellPayload{ 9, 9 });
  REQUIRE(first.get() == second.get());
  REQUIRE(second->get().m_value == 5);

  // another name in the same segment is another instance
  auto other =
    shm::SharedMemoryRepository<CellPayload>::of(fixture.service, fixture.segment, "of-other", CellPayload{ 1, 2 });
  REQUIRE(other.get() != first.get());
  REQUIRE(other->get().m_value == 1);

  // the key belongs to one payload type: a second type is refused instead of
  // being handed a reinterpreted instance
  REQUIRE_THROWS_AS(
    (shm::SharedMemoryRepository<int>::of(fixture.service, fixture.segment, "of-cell", 1)), std::invalid_argument);

  // remove() releases the entry; a later factory call builds a new instance
  second.reset();
  const long referencesBeforeRemove = first.use_count();// first + the registry
  REQUIRE(registry.remove(cellKey));
  REQUIRE_FALSE(registry.contains(cellKey));
  REQUIRE(first.use_count() == referencesBeforeRemove - 1);
  auto third =
    shm::SharedMemoryRepository<CellPayload>::of(fixture.service, fixture.segment, "of-cell", CellPayload{ 7, 8 });
  REQUIRE(third.get() != first.get());
  // the binding is new, the cell is not: it still lives in the segment with the
  // value the first construction stored
  REQUIRE(third->get().m_value == 5);

  // do not leave entries behind: they would keep the fixture's service (and
  // with it the segment mapping) alive beyond the test
  REQUIRE(registry.remove(cellKey));
  REQUIRE(registry.remove(otherKey));
}

}// namespace lintel
