#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "base_library/features/base/events/Event.h"
#include "base_library/features/base/events/EventBus.h"

namespace {

/** Simple struct that is exchanged through the event bus payload. */
struct SensorReading {
    char sensor[24];
    double value;
    std::int64_t measuredAt;
};

std::string shmPath() {
    return (std::filesystem::temp_directory_path() / "event_bus_example.bin")
            .string();
}

}  // namespace

/** Wait up to the given number of attempts for a subscriber event. */
std::optional<Event> receivePolling(EventBus *bus, const std::string &subscriber,
                                    int attempts = 200) {
    for (int attempt = 0; attempt < attempts; ++attempt) {
        if (auto event = bus->receiveOf(subscriber)) {
            return event;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return std::nullopt;
}

int main() {
    try {
        EventBusConfig config;
        config.busCapacity = 8;
        config.subscriberCapacity = 8;
        config.maxTopics = 8;
        config.maxSubscribers = 8;

        const std::string path = shmPath();
        std::filesystem::remove(path);
        boost::interprocess::managed_mapped_file segment(
                boost::interprocess::open_or_create, path.c_str(),
                EventBus::requiredSize(config) + 4096u);

        EventBus *bus = segment.find_or_construct<EventBus>("main")(
                "main", config, segment.get_segment_manager());

        std::cout << "event bus '" << bus->getName() << "' lock free: "
                  << (bus->isLockFree() ? "yes" : "no") << "\n";

        pid_t pid = ::fork();
        if (pid < 0) {
            std::cerr << "fork failed\n";
            return 1;
        }

        if (pid == 0) {
            // child: acts as a worker process and consumes events
            if (!bus->registerTopic("sensor")) {
                return 1;
            }
            if (!bus->subscribe("sensor", "worker")) {
                return 1;
            }
            std::cout << "[worker] waiting for sensor events...\n";
            Event ready("p2p", "ready");
            ready.assign(std::string_view("worker is subscribed"));
            bus->publish(ready);
            for (int i = 0; i < 4; ++i) {
                auto event = receivePolling(bus, "worker");
                if (!event) {
                    std::cerr << "[worker] no event received\n";
                    return 1;
                }
                const SensorReading reading = event->as<SensorReading>();
                std::cout << "[worker] received #" << event->sequence() << " on '"
                          << event->topic() << "': " << reading.sensor
                          << " = " << reading.value << "\n";
            }
            return 0;
        }

        // parent: producer - wait until the worker is subscribed before publishing
        while (!bus->receive()) {
            // the worker signals readiness via a point-to-point event
        }

        for (int i = 0; i < 4; ++i) {
            Event event("sensor", "reading");
            SensorReading reading{};
            std::snprintf(reading.sensor, sizeof(reading.sensor), "sensor_%d", i);
            reading.value = 20.0 + i;
            reading.measuredAt = std::chrono::system_clock::now()
                                         .time_since_epoch()
                                         .count();
            event.assign(reading);
            event.setTimestamp(std::chrono::system_clock::now()
                                       .time_since_epoch()
                                       .count());
            if (!bus->publishToSubscribers(event)) {
                std::cerr << "publish " << i << " failed\n";
                return 1;
            }
            std::cout << "[main] published sensor event " << (i + 1) << "\n";
        }

        // point-to-point demo
        Event greeting("p2p", "greeting");
        greeting.assign(std::string_view("hello over the shared memory bus"));
        if (!bus->publish(greeting)) {
            std::cerr << "p2p publish failed\n";
            return 1;
        }
        auto received = bus->receive();
        if (received) {
            std::cout << "[main] received p2p event '" << received->type()
                      << "' with sequence " << received->sequence() << "\n";
        }
        int status = 0;
        ::waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            std::cout << "example finished successfully\n";
        } else {
            std::cerr << "worker failed\n";
            return 1;
        }
    } catch (const std::exception &exception) {
        std::cerr << "error: " << exception.what() << "\n";
        return 1;
    }
    return 0;
}
