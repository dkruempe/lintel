#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <unistd.h>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "lintel/core/shm/BoostSegmentAllocator.h"
#include "lintel/features/base/events/EventBus.h"

namespace lintel {
namespace {

  using Segment = boost::interprocess::managed_mapped_file;

  constexpr const char *kTopic = "bench_topic";
  const std::array<std::string, 4> kSubscribers = { "subscriber_0", "subscriber_1", "subscriber_2", "subscriber_3" };

  /**
   * Owns one file-backed shared memory segment with a single bus for the whole
   * benchmark run: creating the bus is expensive (it allocates every queue from
   * the segment) and must not be part of the measured section.
   */
  class EventBusEnvironment
  {
  public:
    EventBusEnvironment()
    {
      m_config.busCapacity = 1024;
      m_config.subscriberCapacity = 1024;
      m_config.maxTopics = 8;
      m_config.maxSubscribers = 8;
      m_path =
        std::filesystem::temp_directory_path() / ("lintel_bench_eventbus_" + std::to_string(::getpid()) + ".bin");
      std::filesystem::remove(m_path);
      const std::size_t size = EventBus::requiredSize(m_config) + 4096u;
      m_segment = std::make_unique<Segment>(boost::interprocess::open_or_create, m_path.c_str(), size);
      m_allocator = std::make_unique<BoostSegmentAllocator>(*m_segment->get_segment_manager());
      m_bus = m_segment->find_or_construct<EventBus>("bus")("bus", m_config, ShmSegmentAccessor(*m_allocator));
      m_bus->registerTopic(kTopic);
      for (const auto &subscriber : kSubscribers) { m_bus->subscribe(kTopic, subscriber); }
    }

    ~EventBusEnvironment()
    {
      m_segment->destroy_ptr(m_bus);
      m_segment.reset();
      std::filesystem::remove(m_path);
    }

    EventBusEnvironment(const EventBusEnvironment &) = delete;
    EventBusEnvironment &operator=(const EventBusEnvironment &) = delete;

    EventBus &bus() { return *m_bus; }

  private:
    EventBusConfig m_config;
    std::filesystem::path m_path;
    std::unique_ptr<Segment> m_segment;
    std::unique_ptr<BoostSegmentAllocator> m_allocator;
    EventBus *m_bus = nullptr;
  };

  EventBusEnvironment &environment()
  {
    static EventBusEnvironment instance;
    return instance;
  }

}// namespace

static void BM_EventBus_PublishReceive(benchmark::State &state)
{
  EventBus &bus = environment().bus();
  Event event(kTopic, "tick");
  event.assign<int64_t>(1);
  for (auto _ : state) {
    bus.publish(event);
    benchmark::DoNotOptimize(bus.receive());
  }
}
BENCHMARK(BM_EventBus_PublishReceive);

static void BM_EventBus_PublishToSubscribers4(benchmark::State &state)
{
  EventBus &bus = environment().bus();
  Event event(kTopic, "tick");
  event.assign<int64_t>(1);
  for (auto _ : state) {
    bus.publishToSubscribers(event);
    for (const auto &subscriber : kSubscribers) { benchmark::DoNotOptimize(bus.receiveOf(subscriber)); }
  }
}
BENCHMARK(BM_EventBus_PublishToSubscribers4);
}// namespace lintel
