#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/EventBusComponent.h"
#include "lintel/features/base/configuration/EventBusEntry.h"
#include "lintel/features/base/configuration/SharedMemorySegmentComponent.h"
#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"
#include "lintel/features/base/events/EventBus.h"
#include "lintel/features/base/services/EventBusService.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryService.h"

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  constexpr std::size_t kSegmentSize = 1u << 20;

  std::filesystem::path ebsUniquePath()
  {
    static unsigned int counter = 0;
    const auto path =
      std::filesystem::temp_directory_path()
      / ("eventbus_service_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
    std::filesystem::remove(path);
    return path;
  }

  std::string ebsUniqueName()
  {
    static unsigned int counter = 0;
    return "eventbus_service_segment_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  EventBusConfig ebsValidConfig()
  {
    EventBusConfig config;
    config.busCapacity = 8;
    config.subscriberCapacity = 8;
    config.maxTopics = 8;
    config.maxSubscribers = 8;
    return config;
  }

  // m_configDirectory must be the first member so CONFIG_DIRECTORY is set before the config is built.
  struct EventBusServiceFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_eventbus_service_cfg" };
    std::filesystem::path path;
    std::string name;
    std::shared_ptr<SharedMemorySegment> segment;
    std::shared_ptr<SharedMemoryService> service;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemorySegmentManager> segmentManager;
    std::shared_ptr<Configuration> config;
    std::shared_ptr<EventBusService> eventBusService;

    EventBusServiceFixture(const std::string &busName = "testBus",
      const EventBusConfig &busConfig = ebsValidConfig(),
      const std::string &segmentNameOverride = "")
      : path(ebsUniquePath()), name(ebsUniqueName())
    {
      segment = std::make_shared<SharedMemorySegment>(path, name, kSegmentSize);
      config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());

      const std::string segmentName = segmentNameOverride.empty() ? name : segmentNameOverride;
      auto busEntry = std::make_shared<EventBusEntry>(type_name<EventBusComponent>(),
        busName,
        segmentName,
        busConfig.busCapacity,
        busConfig.subscriberCapacity,
        busConfig.maxTopics,
        busConfig.maxSubscribers);
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment), busEntry });

      segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
      scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("eventbus-service-test"));
      service = std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("eventbus-service-test"));
      eventBusService = std::make_shared<EventBusService>(
        config, std::make_shared<ProcessName>("eventbus-service-test"), service, segmentManager);
    }

    ~EventBusServiceFixture()
    {
      eventBusService.reset();
      service.reset();
      scheduler.reset();
      std::filesystem::remove(path);
      boost::interprocess::named_semaphore::remove((name + "_remap_sem").c_str());
    }
  };

  /** Allocator that fails on demand so the EventBus construction failure paths can be exercised. */
  class EbsFailingAllocator : public ShmSegmentAccessor::Allocator
  {
  public:
    int m_failAtCall = -1;
    int m_nullAtCall = -1;
    int m_calls = 0;
    std::unordered_set<void *> m_live;

    void *allocate(std::size_t bytes) override
    {
      const int call = m_calls++;
      if (call == m_nullAtCall) { return nullptr; }
      if (call == m_failAtCall) { throw std::bad_alloc(); }
      void *block = std::malloc(bytes);
      m_live.insert(block);
      return block;
    }

    void deallocate(void *pointer) override
    {
      m_live.erase(pointer);
      std::free(pointer);
    }

    ~EbsFailingAllocator() override
    {
      for (void *block : m_live) { std::free(block); }
    }
  };

  EventBusConfig ebsSmallConfig()
  {
    EventBusConfig config;
    config.busCapacity = 2;
    config.subscriberCapacity = 2;
    config.maxTopics = 2;
    config.maxSubscribers = 2;
    return config;
  }

}// namespace

TEST_CASE("EventBusService: onInitialize constructs every configured bus")
{
  EventBusServiceFixture fixture;
  REQUIRE_NOTHROW(fixture.eventBusService->onInitialize());
}

TEST_CASE("EventBusService: of returns a usable view over the shared memory bus")
{
  EventBusServiceFixture fixture;
  auto view = fixture.eventBusService->of("testBus");

  REQUIRE(view != nullptr);
  REQUIRE(std::string(view->getName()) == "testBus");
  REQUIRE(view->getConfig().busCapacity == 8);
  REQUIRE(view->isLockFree());
  REQUIRE(view->registerTopic("topic"));
  REQUIRE(view->isTopicRegistered("topic"));
}

TEST_CASE("EventBusService: of caches the resolved bus for an unchanged generation")
{
  EventBusServiceFixture fixture;

  auto first = fixture.eventBusService->of("testBus");
  auto second = fixture.eventBusService->of("testBus");

  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);
  REQUIRE(std::string(first->getName()) == std::string(second->getName()));
}

TEST_CASE("EventBusService: of re-resolves the bus after the segment generation changes")
{
  EventBusServiceFixture fixture;

  REQUIRE(fixture.eventBusService->of("testBus") != nullptr);
  fixture.service->growOf(fixture.segment, 1u << 20);
  REQUIRE(fixture.service->getGeneration() == 1);

  auto view = fixture.eventBusService->of("testBus");
  REQUIRE(view != nullptr);
  REQUIRE(view->getConfig().busCapacity == 8);
}

TEST_CASE("EventBusService: of throws for an unknown bus name")
{
  EventBusServiceFixture fixture;
  REQUIRE_THROWS_AS(fixture.eventBusService->of("nope"), std::runtime_error);
}

TEST_CASE("EventBusService: of throws when the backing segment is missing")
{
  EventBusServiceFixture fixture("testBus", ebsValidConfig(), "does-not-exist");
  REQUIRE_THROWS_AS(fixture.eventBusService->of("testBus"), std::runtime_error);
}

TEST_CASE("EventBusService: of throws when the bus configuration is invalid")
{
  EventBusConfig invalid = ebsValidConfig();
  invalid.maxSubscribers = 0;
  EventBusServiceFixture fixture("testBus", invalid);
  REQUIRE_THROWS_AS(fixture.eventBusService->of("testBus"), std::runtime_error);
}

TEST_CASE("EventBusService: onInitialize swallows a missing segment and keeps going")
{
  EventBusServiceFixture fixture("testBus", ebsValidConfig(), "does-not-exist");
  REQUIRE_NOTHROW(fixture.eventBusService->onInitialize());
}

TEST_CASE("EventBus: topic and subscriber registry lifecycle")
{
  EventBusServiceFixture fixture;
  auto bus = fixture.eventBusService->of("testBus");

  REQUIRE_FALSE(bus->registerTopic(""));
  REQUIRE_FALSE(bus->subscribe("", "s1"));
  REQUIRE_FALSE(bus->subscribe("topic", ""));

  // unknown topic / unknown subscriber branches
  REQUIRE_FALSE(bus->subscribe("nope", "s1"));
  REQUIRE_FALSE(bus->unsubscribe("nope", "s1"));
  REQUIRE_FALSE(bus->isSubscribed("nope", "s1"));
  REQUIRE(bus->numberOfSubscribersOf("nope") == 0);

  REQUIRE(bus->registerTopic("topic"));
  REQUIRE(bus->registerTopic("topic"));// idempotent
  REQUIRE(bus->isTopicRegistered("topic"));
  REQUIRE(bus->topicCount() == 1);

  REQUIRE_FALSE(bus->unsubscribe("topic", "ghost"));// topic known, subscriber unknown

  REQUIRE(bus->subscribe("topic", "s1"));
  REQUIRE(bus->subscribe("topic", "s1"));// idempotent, still active
  REQUIRE(bus->isSubscribed("topic", "s1"));
  REQUIRE(bus->numberOfSubscribersOf("topic") == 1);
  REQUIRE(bus->subscriberCount() == 1);

  // deactivate the subscriber, then re-subscribe -> reactivates the existing slot
  REQUIRE(bus->unsubscribe("topic", "s1"));
  REQUIRE_FALSE(bus->isSubscribed("topic", "s1"));
  REQUIRE_FALSE(bus->unsubscribe("topic", "s1"));// already removed
  REQUIRE(bus->subscribe("topic", "s1"));
  REQUIRE(bus->isSubscribed("topic", "s1"));
}

TEST_CASE("EventBus: point to point publish and receive")
{
  EventBusServiceFixture fixture;
  auto bus = fixture.eventBusService->of("testBus");

  REQUIRE(bus->isEmpty());
  REQUIRE_FALSE(bus->receive().has_value());

  Event event("topic", "type");
  REQUIRE(bus->publish(event));
  REQUIRE_FALSE(bus->isEmpty());

  auto received = bus->receive();
  REQUIRE(received.has_value());
  REQUIRE(std::string(received->topic()) == "topic");
  REQUIRE(bus->isEmpty());
  REQUIRE(bus->sequenceOf() >= 1);
}

TEST_CASE("EventBus: publishing past the bus capacity drops events")
{
  EventBusServiceFixture fixture;
  auto bus = fixture.eventBusService->of("testBus");

  Event event("topic", "type");
  for (std::size_t i = 0; i < 8; ++i) { REQUIRE(bus->publish(event)); }
  REQUIRE_FALSE(bus->publish(event));// queue is full
  REQUIRE(bus->droppedEventsOf() >= 1);
}

TEST_CASE("EventBus: publishToSubscribers delivers and drops on a full subscriber queue")
{
  EventBusServiceFixture fixture;
  auto bus = fixture.eventBusService->of("testBus");
  REQUIRE(bus->registerTopic("topic"));
  REQUIRE(bus->subscribe("topic", "s1"));

  // unknown topic
  Event unknown("other", "type");
  REQUIRE_FALSE(bus->publishToSubscribers(unknown));

  Event event("topic", "type");
  REQUIRE(bus->publishToSubscribers(event));
  auto received = bus->receiveOf("s1");
  REQUIRE(received.has_value());
  REQUIRE(std::string(received->topic()) == "topic");

  // fill the subscriber queue (capacity 8) and then overflow it
  for (std::size_t i = 0; i < 8; ++i) { REQUIRE(bus->publishToSubscribers(event)); }
  REQUIRE_FALSE(bus->isEmptyOf("s1"));
  const auto droppedBefore = bus->droppedEventsOf();
  REQUIRE_FALSE(bus->publishToSubscribers(event));// delivery dropped, nothing received
  REQUIRE(bus->droppedEventsOf() > droppedBefore);

  bus->reset();
  REQUIRE(bus->isEmptyOf("s1"));

  // unknown subscriber
  REQUIRE_FALSE(bus->receiveOf("ghost").has_value());
  REQUIRE(bus->isEmptyOf("ghost"));
}

TEST_CASE("EventBus: constructor cleans up already built queues when a later allocation fails")
{
  EbsFailingAllocator allocator;
  allocator.m_failAtCall = 2;// bus ring + first subscriber queue ok, the second throws
  ShmSegmentAccessor accessor(allocator);

  REQUIRE_THROWS_AS(EventBus("bus", ebsSmallConfig(), accessor), std::bad_alloc);
}

TEST_CASE("EventBus: queue construction reports an allocation returning null")
{
  EbsFailingAllocator allocator;
  allocator.m_nullAtCall = 0;
  ShmSegmentAccessor accessor(allocator);

  REQUIRE_THROWS_AS(EventBus("bus", ebsSmallConfig(), accessor), std::bad_alloc);
}

}// namespace lintel
