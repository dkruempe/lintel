#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include <boost/interprocess/managed_mapped_file.hpp>

#include "lintel/core/services/SharedMemoryService.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"
#include "lintel/features/base/configuration/SharedMemorySegmentComponent.h"
#include "lintel/features/base/configuration/SharedMemorySegmentEntry.h"
#include "lintel/features/base/events/BoostSegmentAllocator.h"
#include "lintel/features/base/events/Event.h"
#include "lintel/features/base/events/EventBus.h"
#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/property/configuration/PropertyRepositoryComponent.h"
#include "lintel/features/property/configuration/PropertyRepositoryEntry.h"
#include "lintel/features/property/events/PropertyChange.h"
#include "lintel/features/property/models/Property.h"
#include "lintel/features/property/repositories/SharedMemoryPropertyRepository.h"
#include "lintel/features/property/services/PropertyService.h"

namespace {

constexpr std::int32_t INITIAL_SPEED = 10;
constexpr std::string_view INITIAL_MESSAGE = "hello worker";

std::string shmPath() {
    return (std::filesystem::temp_directory_path() /
            "property_change_example.bin")
            .string();
}

std::string propertyShmPath() {
    return (std::filesystem::temp_directory_path() /
            "property_change_example_shm.bin")
            .string();
}

}  // namespace

/** Wires the shared memory property repository for the current process. */
class ShmPropertyFixture {
private:
    std::shared_ptr<Configuration> m_configuration;
    std::shared_ptr<ProcessName> m_processName;
    std::shared_ptr<SchedulerService> m_scheduler;
    std::shared_ptr<SharedMemorySegmentManager> m_segmentManager;
    std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
    std::shared_ptr<SharedMemoryPropertyRepository> m_repository;

public:
    explicit ShmPropertyFixture(const std::string &segmentPath) {
        auto environmentConfiguration =
                std::make_shared<EnvironmentConfiguration>();
        environmentConfiguration->overrides(
                EnvironmentConfiguration::ConfigDirectory,
                std::filesystem::temp_directory_path().string() + "/nonexistent");
        m_configuration = std::make_shared<Configuration>(
                std::vector<std::shared_ptr<Component>>{},
                std::move(environmentConfiguration));
        m_configuration->setEntries({
                std::make_shared<SharedMemorySegmentEntry>(
                        type_name<SharedMemorySegmentComponent>(),
                        std::make_shared<SharedMemorySegment>(
                                segmentPath, "shm_property", 5 * 1024 * 1024)),
                std::make_shared<PropertyRepositoryEntry>(
                        type_name<PropertyRepositoryComponent>(),
                        PropertyRepositoryType::SHM_REPOSITORY, true, false),
        });
        m_processName = std::make_shared<ProcessName>("property_change_example");
        m_scheduler = std::make_shared<SchedulerService>(m_processName);
        m_segmentManager =
                std::make_shared<SharedMemorySegmentManager>(m_configuration);
        m_sharedMemoryService = std::make_shared<SharedMemoryService>(
                m_segmentManager, m_scheduler, m_processName);
        m_repository = std::make_shared<SharedMemoryPropertyRepository>(
                m_sharedMemoryService, m_segmentManager, m_configuration,
                m_processName);
    }

    std::shared_ptr<SharedMemoryPropertyRepository> repository() {
        return m_repository;
    }
};

/** Wait up to the given number of attempts for a point-to-point event. */
std::optional<Event> receivePolling(EventBus *bus, int attempts = 200) {
    for (int attempt = 0; attempt < attempts; ++attempt) {
        if (auto event = bus->receive()) {
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
        std::filesystem::remove(propertyShmPath());
        boost::interprocess::managed_mapped_file segment(
                boost::interprocess::open_or_create, path.c_str(),
                EventBus::requiredSize(config) + 4096u);

        BoostSegmentAllocator allocator(*segment.get_segment_manager());
        EventBus *bus = segment.find_or_construct<EventBus>("main")(
                "main", config, ShmSegmentAccessor(allocator));

        pid_t pid = ::fork();
        if (pid < 0) {
            std::cerr << "fork failed\n";
            return 1;
        }

        if (pid == 0) {
            // worker process: no HTTP, just local properties + change listener
            ShmPropertyFixture shmFixture(propertyShmPath());
            auto changeBus = std::make_shared<EventBusView>(*bus);
            PropertyService propertyService({shmFixture.repository()}, {});
            propertyService.setPropertyChangeBus(changeBus, "worker");
            auto maxSpeed = propertyService.getOrCreate<int32_t>(
                    "maxSpeed", "default", "MotorController", "main",
                    "maximum speed of the motor controller", true, INITIAL_SPEED);
            auto message = propertyService.getOrCreate<std::string>(
                    "message", "default", "MotorController", "main",
                    "operator message", true, std::string(INITIAL_MESSAGE));
            std::cout << "[worker] local maxSpeed = " << maxSpeed->getValue()
                      << " (lock-free read: "
                      << (Property<int32_t>::isLockFree() ? "yes" : "no")
                      << "), message = '" << message->getValue() << "'\n";

            Event ready("p2p", "ready");
            ready.assign(std::string_view("worker is listening"));
            bus->publish(ready);

            for (int i = 0; i < 4; ++i) {
                while (propertyService.applyChangeNotifications() == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                std::cout << "[worker] applied change #" << (i + 1)
                          << ": maxSpeed = " << maxSpeed->getValue()
                          << ", message = '" << message->getValue() << "'\n";
            }
            return 0;
        }

        // main process: loads the property, later via HTTP
        ShmPropertyFixture shmFixture(propertyShmPath());
        auto changeBus = std::make_shared<EventBusView>(*bus);
        PropertyService propertyService({shmFixture.repository()}, {});
        propertyService.setPropertyChangeBus(changeBus, "main");
        auto maxSpeed = propertyService.getOrCreate<int32_t>(
                "maxSpeed", "default", "MotorController", "main",
                "maximum speed of the motor controller", true, INITIAL_SPEED);
        auto message = propertyService.getOrCreate<std::string>(
                "message", "default", "MotorController", "main",
                "operator message", true, std::string(INITIAL_MESSAGE));

        if (!receivePolling(bus)) {
            std::cerr << "[main] worker never became ready\n";
            return 1;
        }

        for (const std::int32_t value: {30, 40, 50}) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            propertyService.changeValueOf<int32_t>(maxSpeed, value);
            std::cout << "[main] changed maxSpeed to " << value << "\n";
        }

        // a value that does not fit into the event payload (longer than 126
        // characters) is transported through the shared memory mirror
        const std::string longMessage =
                "emergency stop triggered by PLC at station 42 with error code "
                "0x17A2 - please check the conveyor belt sensor and restart the "
                "safety circuit before resuming production.";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        propertyService.changeStringValueOf(message, longMessage);
        std::cout << "[main] changed message to " << longMessage.size()
                  << " characters (truncated in the event, full value via "
                     "shared memory)\n";

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
