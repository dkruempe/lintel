#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <type_traits>
#include <unistd.h>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "base_library/features/base/events/Event.h"
#include "base_library/features/base/events/EventBus.h"

#include <catch2/catch_all.hpp>

namespace {

using Segment = boost::interprocess::managed_mapped_file;

std::filesystem::path uniqueShmPath() {
    static unsigned int counter = 0;
    const auto path = std::filesystem::temp_directory_path() /
                      ("eventbus_test_" + std::to_string(::getpid()) + "_" +
                       std::to_string(counter++) + ".bin");
    std::filesystem::remove(path);
    return path;
}

struct BusFixture {
    EventBusConfig config;
    std::filesystem::path path;
    std::size_t size;
    Segment segment;
    EventBus *bus;

    explicit BusFixture(EventBusConfig fixtureConfig)
        : config(fixtureConfig),
          path(uniqueShmPath()),
          size(EventBus::requiredSize(config) + 4096u),
          segment(boost::interprocess::open_or_create, path.c_str(), size) {
        bus = segment.find_or_construct<EventBus>("bus")(
                "bus", config, segment.get_segment_manager());
    }

    ~BusFixture() {
        segment.destroy_ptr(bus);
        segment.flush();
        std::filesystem::remove(path);
    }
};

EventBusConfig defaultConfig() {
    EventBusConfig config;
    config.busCapacity = 8;
    config.subscriberCapacity = 8;
    config.maxTopics = 8;
    config.maxSubscribers = 8;
    return config;
}

}  // namespace

TEST_CASE("EventBus: point-to-point publish and receive") {
    BusFixture fixture(defaultConfig());
    auto *bus = fixture.bus;

    REQUIRE(std::string(bus->getName()) == "bus");
    REQUIRE(bus->isLockFree());
    REQUIRE(bus->isEmpty());

    Event event("p2p", "ping");
    event.assign(42);
    REQUIRE(bus->publish(event));
    REQUIRE_FALSE(bus->isEmpty());

    auto received = bus->receive();
    REQUIRE(received.has_value());
    REQUIRE(received->as<int>() == 42);
    REQUIRE(std::strcmp(received->topic(), "p2p") == 0);
    REQUIRE(std::strcmp(received->type(), "ping") == 0);
    REQUIRE(received->sequence() == 1);
    REQUIRE(bus->isEmpty());
    REQUIRE_FALSE(bus->receive().has_value());
}

TEST_CASE("EventBus: publish assigns monotonically increasing sequence") {
    BusFixture fixture(defaultConfig());
    auto *bus = fixture.bus;

    for (int i = 0; i < 5; ++i) {
        REQUIRE(bus->publish(Event("seq", "tick")));
    }
    REQUIRE(bus->sequenceOf() == 5);
    for (int i = 1; i <= 5; ++i) {
        auto event = bus->receive();
        REQUIRE(event.has_value());
        REQUIRE(event->sequence() == i);
    }
}

TEST_CASE("EventBus: bounded bus queue drops events when full") {
    EventBusConfig config = defaultConfig();
    config.busCapacity = 4;
    BusFixture fixture(config);
    auto *bus = fixture.bus;

    for (int i = 0; i < 4; ++i) {
        REQUIRE(bus->publish(Event("p2p", "fill")));
    }
    REQUIRE_FALSE(bus->publish(Event("p2p", "overflow")));
    REQUIRE(bus->droppedEventsOf() == 1);
    REQUIRE(bus->sequenceOf() == 5);

    for (int i = 0; i < 4; ++i) {
        REQUIRE(bus->receive().has_value());
    }
    REQUIRE_FALSE(bus->receive().has_value());
}

TEST_CASE("EventBus: topic and subscriber registry") {
    BusFixture fixture(defaultConfig());
    auto *bus = fixture.bus;

    REQUIRE_FALSE(bus->isTopicRegistered("sensor"));
    REQUIRE(bus->registerTopic("sensor"));
    REQUIRE(bus->isTopicRegistered("sensor"));
    REQUIRE(bus->registerTopic("sensor"));  // idempotent
    REQUIRE(bus->topicCount() == 1);

    REQUIRE_FALSE(bus->subscribe("unknown", "worker"));
    REQUIRE(bus->subscribe("sensor", "worker"));
    REQUIRE(bus->isSubscribed("sensor", "worker"));
    REQUIRE(bus->subscribe("sensor", "worker"));  // idempotent
    REQUIRE(bus->subscribe("sensor", "archiver"));
    REQUIRE(bus->numberOfSubscribersOf("sensor") == 2);
    REQUIRE(bus->subscriberCount() == 2);

    REQUIRE(bus->unsubscribe("sensor", "worker"));
    REQUIRE_FALSE(bus->isSubscribed("sensor", "worker"));
    REQUIRE(bus->numberOfSubscribersOf("sensor") == 1);
    REQUIRE_FALSE(bus->unsubscribe("sensor", "worker"));
}

TEST_CASE("EventBus: registry respects configured limits") {
    EventBusConfig config = defaultConfig();
    config.maxTopics = 2;
    config.maxSubscribers = 1;
    BusFixture fixture(config);
    auto *bus = fixture.bus;

    REQUIRE(bus->registerTopic("a"));
    REQUIRE(bus->registerTopic("b"));
    REQUIRE_FALSE(bus->registerTopic("c"));

    REQUIRE(bus->subscribe("a", "one"));
    REQUIRE_FALSE(bus->subscribe("a", "two"));
    REQUIRE(bus->numberOfSubscribersOf("a") == 1);
}

TEST_CASE("EventBus: publish to subscribers broadcasts per topic") {
    BusFixture fixture(defaultConfig());
    auto *bus = fixture.bus;

    REQUIRE(bus->registerTopic("sensor"));
    REQUIRE(bus->registerTopic("alarm"));
    REQUIRE(bus->subscribe("sensor", "worker"));
    REQUIRE(bus->subscribe("sensor", "archiver"));
    REQUIRE(bus->subscribe("alarm", "worker"));

    Event sensor("sensor", "reading");
    sensor.assign(36.5);
    REQUIRE(bus->publishToSubscribers(sensor));

    Event alarm("alarm", "critical");
    alarm.assign(true);
    REQUIRE(bus->publishToSubscribers(alarm));

    // worker gets sensor + alarm, archiver only sensor
    auto workerFirst = bus->receiveOf("worker");
    REQUIRE(workerFirst.has_value());
    REQUIRE(std::strcmp(workerFirst->topic(), "sensor") == 0);
    auto workerSecond = bus->receiveOf("worker");
    REQUIRE(workerSecond.has_value());
    REQUIRE(std::strcmp(workerSecond->topic(), "alarm") == 0);
    REQUIRE_FALSE(bus->receiveOf("worker").has_value());

    auto archiver = bus->receiveOf("archiver");
    REQUIRE(archiver.has_value());
    REQUIRE(std::strcmp(archiver->topic(), "sensor") == 0);
    REQUIRE(archiver->as<double>() == 36.5);
    REQUIRE_FALSE(bus->receiveOf("archiver").has_value());

    REQUIRE(bus->isEmptyOf("worker"));
    REQUIRE(bus->isEmptyOf("unknown"));  // unknown subscribers count as empty
}

TEST_CASE("EventBus: publish to unknown topic delivers nothing") {
    BusFixture fixture(defaultConfig());
    auto *bus = fixture.bus;

    REQUIRE_FALSE(bus->publishToSubscribers(Event("ghost", "x")));
}

TEST_CASE("EventBus: reset drains all queues") {
    BusFixture fixture(defaultConfig());
    auto *bus = fixture.bus;

    REQUIRE(bus->registerTopic("t"));
    REQUIRE(bus->subscribe("t", "sub"));
    for (int i = 0; i < 3; ++i) {
        REQUIRE(bus->publish(Event("p2p", "x")));
        REQUIRE(bus->publishToSubscribers(Event("t", "x")));
    }
    bus->reset();
    REQUIRE(bus->isEmpty());
    REQUIRE(bus->isEmptyOf("sub"));
    REQUIRE_FALSE(bus->receive().has_value());
    REQUIRE_FALSE(bus->receiveOf("sub").has_value());
}

TEST_CASE("EventBus: IPC across independent mappings") {
    EventBusConfig config = defaultConfig();
    const std::filesystem::path path = uniqueShmPath();
    std::filesystem::remove(path);

    const std::size_t size = EventBus::requiredSize(config) + 4096u;
    Segment mappingA(boost::interprocess::open_or_create, path.c_str(), size);
    Segment mappingB(boost::interprocess::open_only, path.c_str());

    auto *managerA = mappingA.get_segment_manager();
    auto *managerB = mappingB.get_segment_manager();
    EventBus *busA = mappingA.find_or_construct<EventBus>("bus")(
            "bus", config, managerA);
    EventBus *busB = mappingB.find_or_construct<EventBus>("bus")(
            "bus", config, managerB);

    REQUIRE(busA != busB);

    // producer through mapping A, consumer through mapping B
    for (int i = 0; i < 4; ++i) {
        Event event("ipc", "ping");
        event.assign(i * 3);
        REQUIRE(busA->publish(event));
    }
    for (int i = 0; i < 4; ++i) {
        auto event = busB->receive();
        REQUIRE(event.has_value());
        REQUIRE(event->as<int>() == i * 3);
    }

    // pub/sub through different mappings
    REQUIRE(busA->registerTopic("ipc-topic"));
    REQUIRE(busA->subscribe("ipc-topic", "ipc-sub"));
    REQUIRE(busB->isSubscribed("ipc-topic", "ipc-sub"));
    Event event("ipc-topic", "broadcast");
    event.assign(7.25);
    REQUIRE(busA->publishToSubscribers(event));
    auto received = busB->receiveOf("ipc-sub");
    REQUIRE(received.has_value());
    REQUIRE(received->as<double>() == 7.25);

    REQUIRE(busA->sequenceOf() == busB->sequenceOf());

    // the two mappings reference the same object, so destroy it only once
    mappingA.destroy_ptr(busA);
    mappingA.flush();
    std::filesystem::remove(path);
}

TEST_CASE("EventBus: requiredSize") {
    EventBusConfig config;
    config.busCapacity = 16;
    config.subscriberCapacity = 16;
    config.maxTopics = 4;
    config.maxSubscribers = 4;
    const std::size_t required = EventBus::requiredSize(config);
    REQUIRE(required > 0);
    REQUIRE(required > sizeof(EventBus));

    EventBusConfig invalid;
    invalid.busCapacity = 0;
    REQUIRE(EventBus::requiredSize(invalid) == 0);
}

TEST_CASE("EventBus: invalid configuration is rejected") {
    EventBusConfig config;
    config.busCapacity = 0;
    REQUIRE_FALSE(config.isValid());

    const std::filesystem::path path = uniqueShmPath();
    std::filesystem::remove(path);
    Segment segment(boost::interprocess::open_or_create, path.c_str(),
                    EventBus::requiredSize(defaultConfig()) + 4096u);
    REQUIRE_THROWS_AS(segment.find_or_construct<EventBus>("bad")(
                              "bad", config, segment.get_segment_manager()),
                      std::invalid_argument);
    std::filesystem::remove(path);
}

TEST_CASE("Event: content size limits and round trip") {
    Event event("topic", "type");
    struct Payload {
        char text[16];
        int number;
        double ratio;
    };
    Payload payload{};
    std::strcpy(payload.text, "hello");
    payload.number = 7;
    payload.ratio = 2.5;
    event.assign(payload);
    REQUIRE(event.contentLength() == sizeof(Payload));
    REQUIRE_FALSE(event.isContentEmpty());

    const auto back = event.as<Payload>();
    REQUIRE(std::strcmp(back.text, "hello") == 0);
    REQUIRE(back.number == 7);
    REQUIRE(back.ratio == 2.5);

    event.setContent(reinterpret_cast<const std::byte *>(payload.text),
                     sizeof(payload.text));
    REQUIRE(event.contentLength() == sizeof(payload.text));
    REQUIRE(std::memcmp(event.content(), payload.text, sizeof(payload.text)) ==
            0);

    REQUIRE(std::is_trivially_copyable_v<Event>);
    REQUIRE(std::is_trivially_copyable_v<Payload>);
}
