#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../helpers/PlatformCompat.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/configuration/EnvironmentConfiguration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/models/SharedMemorySegment.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/SharedMemorySegmentComponent.h"
#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"
#include "lintel/features/base/msg/MessageQueueBackend.h"
#include "lintel/features/base/msg/MessageQueueCoreFactory.h"
#include "lintel/features/base/msg/MessageQueueShmCore.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryService.h"

#include <catch2/catch_all.hpp>

#include "../helpers/ScopedEnvironmentVariable.h"

namespace lintel {
namespace {

  constexpr std::size_t kQueueSegmentSize = 1u << 20;

  std::filesystem::path uniqueQueueShmPath()
  {
    static unsigned int counter = 0;
    return std::filesystem::temp_directory_path()
           / ("shm_queue_test_" + std::to_string(::getpid()) + "_" + std::to_string(counter++) + ".bin");
  }

  std::string uniqueQueueSegmentName()
  {
    static unsigned int counter = 0;
    return "shm_queue_" + std::to_string(::getpid()) + "_" + std::to_string(counter++);
  }

  std::atomic<unsigned int> g_queueCounter{ 0 };

  std::string uniqueQueueName()
  {
    return "shm_queue_core_" + std::to_string(::getpid()) + "_" + std::to_string(++g_queueCounter);
  }

  struct TestMessage
  {
    std::int32_t value = 0;
    std::int32_t sequence = 0;
  };

  /** Wires a SharedMemoryService over one real segment, like a queue user would. */
  struct QueueFixture
  {
    ScopedEnvironmentVariable m_configDirectory{ "CONFIG_DIRECTORY", "/tmp/nonexistent_shm_queue_cfg" };
    std::filesystem::path path;
    std::string segmentName;
    std::shared_ptr<SharedMemorySegment> segment;
    std::shared_ptr<SchedulerService> scheduler;
    std::shared_ptr<SharedMemorySegmentManager> segmentManager;
    std::shared_ptr<SharedMemoryService> service;

    QueueFixture() : path(uniqueQueueShmPath()), segmentName(uniqueQueueSegmentName())
    {
      segment = std::make_shared<SharedMemorySegment>(path, segmentName, kQueueSegmentSize);
      auto config = std::make_shared<Configuration>(
        std::vector<std::shared_ptr<Component>>{}, std::make_shared<EnvironmentConfiguration>());
      config->setEntries(
        { std::make_shared<SharedMemorySegmentEntry>(type_name<SharedMemorySegmentComponent>(), segment) });
      segmentManager = std::make_shared<SharedMemorySegmentManager>(config);
      scheduler = std::make_shared<SchedulerService>(std::make_shared<ProcessName>("shm-queue-test"));
      service = std::make_shared<SharedMemoryService>(
        segmentManager, scheduler, std::make_shared<ProcessName>("shm-queue-test"));
    }

    ~QueueFixture()
    {
      service.reset();
      scheduler.reset();
      std::filesystem::remove(path);
      boost::interprocess::named_semaphore::remove((segmentName + "_remap_sem").c_str());
    }

    [[nodiscard]] ShmSegmentAccessor accessor() { return service->getSegmentAccessor(segment); }
  };

  /** Removes the registry association before the fixture tears the segment down. */
  class QueueCleanup
  {
  public:
    explicit QueueCleanup(std::string name) : m_name(std::move(name)) {}

    ~QueueCleanup() { MessageQueueShmCore::removeOf(m_name); }

    QueueCleanup(const QueueCleanup &) = delete;
    QueueCleanup &operator=(const QueueCleanup &) = delete;

  private:
    std::string m_name;
  };

}// namespace

TEST_CASE("MessageQueueShmCore sends and receives FIFO")
{
  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();
  QueueCleanup cleanup(name);
  MessageQueueShmCore queue(name, 4, sizeof(TestMessage), accessor);

  REQUIRE(queue.getName() == name);
  REQUIRE(queue.getMsgCount() == 4);
  REQUIRE(queue.getMsgSize() == sizeof(TestMessage));
  REQUIRE(queue.numberMessagesOf() == 0);

  for (std::int32_t index = 0; index < 3; ++index) {
    const TestMessage message{ index * 10, index };
    queue.sendOf(&message, sizeof(message));
  }
  REQUIRE(queue.numberMessagesOf() == 3);

  for (std::int32_t index = 0; index < 3; ++index) {
    TestMessage received{};
    queue.receiveOf(&received, sizeof(received));
    REQUIRE(received.value == index * 10);
    REQUIRE(received.sequence == index);
  }
  REQUIRE(queue.numberMessagesOf() == 0);
}

TEST_CASE("MessageQueueShmCore try semantics and full/empty state")
{
  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();
  QueueCleanup cleanup(name);
  MessageQueueShmCore queue(name, 2, sizeof(TestMessage), accessor);

  TestMessage message{};
  REQUIRE_FALSE(queue.tryReceiveOf(&message, sizeof(message)));

  REQUIRE(queue.trySendOf(&message, sizeof(message)));
  message.value = 1;
  REQUIRE(queue.trySendOf(&message, sizeof(message)));
  message.value = 2;
  REQUIRE_FALSE(queue.trySendOf(&message, sizeof(message)));// full
  REQUIRE(queue.numberMessagesOf() == 2);

  TestMessage received{};
  REQUIRE(queue.tryReceiveOf(&received, sizeof(received)));
  REQUIRE(received.value == 0);
  REQUIRE(queue.trySendOf(&message, sizeof(message)));// slot free again

  REQUIRE(queue.tryReceiveOf(&received, sizeof(received)));
  REQUIRE(received.value == 1);
  REQUIRE(queue.tryReceiveOf(&received, sizeof(received)));
  REQUIRE(received.value == 2);
  REQUIRE_FALSE(queue.tryReceiveOf(&received, sizeof(received)));

  // wrap-around: more send/receive cycles than the capacity
  for (std::int32_t index = 0; index < 7; ++index) {
    TestMessage cycle{ index, 0 };
    REQUIRE(queue.trySendOf(&cycle, sizeof(cycle)));
    REQUIRE(queue.tryReceiveOf(&received, sizeof(received)));
    REQUIRE(received.value == index);
  }
}

TEST_CASE("MessageQueueShmCore honours bounded waits")
{
  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();
  QueueCleanup cleanup(name);
  MessageQueueShmCore queue(name, 1, sizeof(TestMessage), accessor);

  TestMessage message{ 1, 1 };
  TestMessage received{};
  REQUIRE_FALSE(queue.receiveOfWithTimeout(&received, sizeof(received), std::chrono::milliseconds(10)));

  REQUIRE(queue.sendOfWithTimeout(&message, sizeof(message), std::chrono::milliseconds(10)));
  REQUIRE(queue.numberMessagesOf() == 1);

  message.value = 2;
  REQUIRE_FALSE(queue.sendOfWithTimeout(&message, sizeof(message), std::chrono::milliseconds(10)));
  REQUIRE(queue.numberMessagesOf() == 1);

  REQUIRE(queue.receiveOfWithTimeout(&received, sizeof(received), std::chrono::milliseconds(10)));
  REQUIRE(received.value == 1);
}

TEST_CASE("MessageQueueShmCore blocking receive wakes up on send")
{
  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();
  QueueCleanup cleanup(name);
  MessageQueueShmCore queue(name, 2, sizeof(TestMessage), accessor);

  TestMessage received{};
  std::atomic<bool> receivedFlag{ false };
  std::thread consumer([&queue, &received, &receivedFlag] {
    queue.receiveOf(&received, sizeof(received));
    receivedFlag.store(true, std::memory_order_relaxed);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  TestMessage message{ 42, 7 };
  queue.sendOf(&message, sizeof(message));
  consumer.join();

  REQUIRE(receivedFlag.load(std::memory_order_relaxed));
  REQUIRE(received.value == 42);
  REQUIRE(received.sequence == 7);
}

TEST_CASE("MessageQueueShmCore applies the overflow policies")
{
  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();
  const std::string deadLetterName = name + ".dlq";
  QueueCleanup cleanup(name);
  QueueCleanup deadLetterCleanup(deadLetterName);
  MessageQueueShmCore queue(name, 2, sizeof(TestMessage), accessor);

  TestMessage first{ 1, 1 };
  TestMessage second{ 2, 2 };
  TestMessage third{ 3, 3 };
  REQUIRE(queue.trySendOf(&first, sizeof(first)));
  REQUIRE(queue.trySendOf(&second, sizeof(second)));

  // Fail (default): the new message is rejected and the queue is unchanged
  queue.setOverflowPolicy(MessageQueueOverflowPolicy::Fail);
  REQUIRE_FALSE(queue.trySendOf(&third, sizeof(third)));
  REQUIRE(queue.numberMessagesOf() == 2);

  // DropNewest: same observable result for the queue content
  queue.setOverflowPolicy(MessageQueueOverflowPolicy::DropNewest);
  REQUIRE_FALSE(queue.trySendOf(&third, sizeof(third)));
  REQUIRE(queue.numberMessagesOf() == 2);

  // DropOldest: the oldest message is replaced by the new one
  queue.setOverflowPolicy(MessageQueueOverflowPolicy::DropOldest);
  REQUIRE(queue.trySendOf(&third, sizeof(third)));
  REQUIRE(queue.numberMessagesOf() == 2);
  TestMessage received{};
  REQUIRE(queue.tryReceiveOf(&received, sizeof(received)));
  REQUIRE(received.value == 2);
  REQUIRE(queue.tryReceiveOf(&received, sizeof(received)));
  REQUIRE(received.value == 3);

  // BlockWithTimeout: bounded wait, then drop
  queue.setOverflowPolicy(MessageQueueOverflowPolicy::BlockWithTimeout);
  queue.setOverflowTimeout(std::chrono::milliseconds(5));
  REQUIRE(queue.trySendOf(&first, sizeof(first)));
  REQUIRE(queue.trySendOf(&second, sizeof(second)));
  REQUIRE_FALSE(queue.sendOfWithTimeout(&third, sizeof(third), std::chrono::milliseconds(5)));

  // BlockWithTimeout via the configured policy: trySendOf must honour the
  // stored timeout instead of dropping immediately (queue is still full).
  const auto policyStart = std::chrono::steady_clock::now();
  REQUIRE_FALSE(queue.trySendOf(&third, sizeof(third)));
  const auto policyElapsed = std::chrono::steady_clock::now() - policyStart;
  REQUIRE(policyElapsed >= std::chrono::milliseconds(4));
  REQUIRE(queue.numberMessagesOf() == 2);

  // DeadLetter: the message is forwarded to the configured DLQ
  queue.setOverflowPolicy(MessageQueueOverflowPolicy::DeadLetter);
  queue.setDeadLetterQueueName(deadLetterName);
  REQUIRE(queue.trySendOf(&third, sizeof(third)));
  REQUIRE(queue.numberMessagesOf() == 2);
  REQUIRE(MessageQueueShmCore::numberMessagesOfIfExistsOf(deadLetterName) == 1);

  MessageQueueShmCore deadLetter(deadLetterName, 2, sizeof(TestMessage), accessor);
  TestMessage deadLettered{};
  REQUIRE(deadLetter.tryReceiveOf(&deadLettered, sizeof(deadLettered)));
  REQUIRE(deadLettered.value == 3);
}

TEST_CASE("MessageQueueShmCore is visible to a second core of the same segment")
{
  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();
  QueueCleanup cleanup(name);

  MessageQueueShmCore producer(name, 4, sizeof(TestMessage), accessor);
  MessageQueueShmCore consumer(name, 4, sizeof(TestMessage), accessor);

  REQUIRE(MessageQueueShmCore::numberMessagesOfIfExistsOf(name) == 0);
  TestMessage message{ 11, 22 };
  producer.sendOf(&message, sizeof(message));

  REQUIRE(producer.numberMessagesOf() == 1);
  REQUIRE(consumer.numberMessagesOf() == 1);
  REQUIRE(MessageQueueShmCore::numberMessagesOfIfExistsOf(name) == 1);

  TestMessage received{};
  REQUIRE(consumer.tryReceiveOf(&received, sizeof(received)));
  REQUIRE(received.value == 11);
  REQUIRE(received.sequence == 22);
  REQUIRE(producer.numberMessagesOf() == 0);

  REQUIRE_THROWS_AS((MessageQueueShmCore(name, 8, sizeof(TestMessage), accessor)), std::invalid_argument);
}

TEST_CASE("MessageQueueShmCore removeOf drops the queue association")
{
  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();

  {
    MessageQueueShmCore queue(name, 2, sizeof(TestMessage), accessor);
    TestMessage message{ 1, 1 };
    REQUIRE(queue.trySendOf(&message, sizeof(message)));
    REQUIRE(MessageQueueShmCore::numberMessagesOfIfExistsOf(name) == 1);

    MessageQueueShmCore::removeOf(name);
    REQUIRE(MessageQueueShmCore::numberMessagesOfIfExistsOf(name) == 0);
    REQUIRE(queue.numberMessagesOf() == 1);// the existing core keeps its ring
  }
  REQUIRE(MessageQueueShmCore::numberMessagesOfIfExistsOf(name) == 0);
}

TEST_CASE("MessageQueueBackendConfig and factory select the shared memory backend")
{
  const MessageQueueBackend previous = MessageQueueBackendConfig::backend();
  MessageQueueBackendConfig::setBackend(MessageQueueBackend::Shm);
  REQUIRE(MessageQueueBackendConfig::useSharedMemory());
  REQUIRE(MessageQueueBackendConfig::fromString("shm") == MessageQueueBackend::Shm);
  REQUIRE(MessageQueueBackendConfig::fromString("boost") == MessageQueueBackend::Boost);

  QueueFixture fixture;
  const ShmSegmentAccessor accessor = fixture.accessor();
  const std::string name = uniqueQueueName();
  QueueCleanup cleanup(name);

  auto core = MessageQueueCoreFactory::create(name, 2, sizeof(TestMessage), accessor);
  REQUIRE(core != nullptr);
  TestMessage message{ 5, 6 };
  REQUIRE(core->trySendOf(&message, sizeof(message)));
  REQUIRE(core->numberMessagesOf() == 1);
  REQUIRE(MessageQueueCoreFactory::numberMessagesOfIfExistsOf(MessageQueueBackend::Shm, name) == 1);

  TestMessage received{};
  REQUIRE(core->tryReceiveOf(&received, sizeof(received)));
  REQUIRE(received.value == 5);
  REQUIRE(received.sequence == 6);

  MessageQueueCoreFactory::removeOf(MessageQueueBackend::Shm, name);
  MessageQueueBackendConfig::setBackend(previous);
}

}// namespace lintel
