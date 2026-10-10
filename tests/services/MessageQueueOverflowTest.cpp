#include <catch2/catch_all.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include <unistd.h>

#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/msg/Message.h"
#include "lintel/features/base/msg/MessageQueue.h"

namespace {
    std::string uniqueQueueName() {
        static int counter = 0;
        // ctest gives every test case its own process, so the shared memory name needs the PID:
        // without it the cases attach to each other's queue as soon as ctest runs them in parallel.
        return "opencode_overflow_test_" + std::to_string(::getpid()) + "_" + std::to_string(++counter);
    }
}  // namespace

/** Drops the shared memory queue on every exit path, also when a failing REQUIRE aborts the case */
class QueueCleanup
{
public:
  explicit QueueCleanup(std::string name) : m_name(std::move(name)) { MessageQueue<Message>::removeOf(m_name); }

  ~QueueCleanup() { MessageQueue<Message>::removeOf(m_name); }

  QueueCleanup(const QueueCleanup &) = delete;
  QueueCleanup &operator=(const QueueCleanup &) = delete;

private:
  std::string m_name;
};

TEST_CASE("MessageQueue: sendOfWithTimeout queues when space is available") {
    const std::string name = uniqueQueueName();
    QueueCleanup cleanup(name);
    auto processName = std::make_shared<ProcessName>("testProcess");

    MessageQueue<Message> queue("testProcess", name, 4, processName);
    Message msg;

    REQUIRE(queue.sendOfWithTimeout(msg, std::chrono::milliseconds(200)));
    REQUIRE(queue.numberMessagesOf() == 1);
    REQUIRE(queue.droppedMessagesOf() == 0);
}

TEST_CASE("MessageQueue: sendOfWithTimeout drops when queue stays full") {
    const std::string name = uniqueQueueName();
    QueueCleanup cleanup(name);
    auto processName = std::make_shared<ProcessName>("testProcess");

    MessageQueue<Message> queue("testProcess", name, 1, processName);

    Message msg;
    REQUIRE(queue.trySendOf(msg));
    REQUIRE(queue.numberMessagesOf() == 1);

    REQUIRE_FALSE(queue.sendOfWithTimeout(msg, std::chrono::milliseconds(50)));
    REQUIRE(queue.droppedMessagesOf() == 1);
    REQUIRE(queue.numberMessagesOf() == 1);
}
