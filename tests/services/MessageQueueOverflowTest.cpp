#include <catch2/catch_all.hpp>

#include <chrono>
#include <memory>
#include <string>

#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/msg/Message.h"
#include "lintel/features/base/msg/MessageQueue.h"

namespace {
    std::string uniqueQueueName() {
        static int counter = 0;
        return "opencode_overflow_test_" + std::to_string(++counter);
    }
}  // namespace

TEST_CASE("MessageQueue: sendOfWithTimeout queues when space is available") {
    const std::string name = uniqueQueueName();
    MessageQueue<Message>::removeOf(name);
    auto processName = std::make_shared<ProcessName>("testProcess");

    MessageQueue<Message> queue("testProcess", name, 4, processName);
    Message msg;

    REQUIRE(queue.sendOfWithTimeout(msg, std::chrono::milliseconds(200)));
    REQUIRE(queue.numberMessagesOf() == 1);
    REQUIRE(queue.droppedMessagesOf() == 0);

    MessageQueue<Message>::removeOf(name);
}

TEST_CASE("MessageQueue: sendOfWithTimeout drops when queue stays full") {
    const std::string name = uniqueQueueName();
    MessageQueue<Message>::removeOf(name);
    auto processName = std::make_shared<ProcessName>("testProcess");

    MessageQueue<Message> queue("testProcess", name, 1, processName);

    Message msg;
    REQUIRE(queue.trySendOf(msg));
    REQUIRE(queue.numberMessagesOf() == 1);

    REQUIRE_FALSE(queue.sendOfWithTimeout(msg, std::chrono::milliseconds(50)));
    REQUIRE(queue.droppedMessagesOf() == 1);
    REQUIRE(queue.numberMessagesOf() == 1);

    MessageQueue<Message>::removeOf(name);
}
