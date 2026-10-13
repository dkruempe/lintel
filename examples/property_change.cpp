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

std::string shmPath() { return (std::filesystem::temp_directory_path() / "property_change_example.bin").string(); }

std::string propertyShmPath()
{
  return (std::filesystem::temp_directory_path() / "property_change_example_shm.bin").string();
}

}// namespace

/** Wires the shared memory property repository for the current process. */
class ShmPropertyFixture
{
private:
  std::shared_ptr<lintel::Configuration> m_configuration;
  std::shared_ptr<lintel::ProcessName> m_processName;
  std::shared_ptr<lintel::SchedulerService> m_scheduler;
  std::shared_ptr<lintel::SharedMemorySegmentManager> m_segmentManager;
  std::shared_ptr<lintel::SharedMemoryService> m_sharedMemoryService;
  std::shared_ptr<lintel::SharedMemoryPropertyRepository> m_repository;

public:
  explicit ShmPropertyFixture(const std::string &segmentPath)
  {
    auto environmentConfiguration = std::make_shared<lintel::EnvironmentConfiguration>();
    environmentConfiguration->overrides(lintel::EnvironmentConfiguration::ConfigDirectory,
      std::filesystem::temp_directory_path().string() + "/nonexistent");
    m_configuration = std::make_shared<lintel::Configuration>(
      std::vector<std::shared_ptr<lintel::Component>>{}, std::move(environmentConfiguration));
    m_configuration->setEntries({
      std::make_shared<lintel::SharedMemorySegmentEntry>(lintel::type_name<lintel::SharedMemorySegmentComponent>(),
        std::make_shared<lintel::SharedMemorySegment>(segmentPath, "shm_property", 5 * 1024 * 1024)),
      std::make_shared<lintel::PropertyRepositoryEntry>(lintel::type_name<lintel::PropertyRepositoryComponent>(),
        lintel::PropertyRepositoryType::SHM_REPOSITORY,
        true,
        false),
    });
    m_processName = std::make_shared<lintel::ProcessName>("property_change_example");
    m_scheduler = std::make_shared<lintel::SchedulerService>(m_processName);
    m_segmentManager = std::make_shared<lintel::SharedMemorySegmentManager>(m_configuration);
    m_sharedMemoryService = std::make_shared<lintel::SharedMemoryService>(m_segmentManager, m_scheduler, m_processName);
    m_repository = std::make_shared<lintel::SharedMemoryPropertyRepository>(
      m_sharedMemoryService, m_segmentManager, m_configuration, m_processName);
  }

  std::shared_ptr<lintel::SharedMemoryPropertyRepository> repository() { return m_repository; }
};

/** Wait up to the given number of attempts for a point-to-point event. */
std::optional<lintel::Event> receivePolling(lintel::EventBus *bus, int attempts = 200)
{
  for (int attempt = 0; attempt < attempts; ++attempt) {
    if (auto event = bus->receive()) { return event; }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return std::nullopt;
}

int main()
{
  try {
    lintel::EventBusConfig config;
    config.busCapacity = 8;
    config.subscriberCapacity = 8;
    config.maxTopics = 8;
    config.maxSubscribers = 8;

    const std::string path = shmPath();
    std::filesystem::remove(path);
    std::filesystem::remove(propertyShmPath());
    boost::interprocess::managed_mapped_file segment(
      boost::interprocess::open_or_create, path.c_str(), lintel::EventBus::requiredSize(config) + 4096u);

    lintel::BoostSegmentAllocator allocator(*segment.get_segment_manager());
    lintel::EventBus *bus =
      segment.find_or_construct<lintel::EventBus>("main")("main", config, lintel::ShmSegmentAccessor(allocator));

    pid_t pid = ::fork();
    if (pid < 0) {
      std::cerr << "fork failed\n";
      return 1;
    }

    if (pid == 0) {
      // worker process: no HTTP, just local properties + change listener
      ShmPropertyFixture shmFixture(propertyShmPath());
      auto changeBus = std::make_shared<lintel::EventBusView>(*bus);
      lintel::PropertyService propertyService({ shmFixture.repository() }, {});
      propertyService.setPropertyChangeBus(changeBus, "worker");
      auto maxSpeed = propertyService.getOrCreate<int32_t>(
        "maxSpeed", "default", "MotorController", "main", "maximum speed of the motor controller", true, INITIAL_SPEED);
      auto message = propertyService.getOrCreate<std::string>(
        "message", "default", "MotorController", "main", "operator message", true, std::string(INITIAL_MESSAGE));
      std::cout << "[worker] local maxSpeed = " << maxSpeed->getValue()
                << " (lock-free read: " << (lintel::Property<int32_t>::isLockFree() ? "yes" : "no") << "), message = '"
                << message->getValue() << "'\n";

      lintel::Event ready("p2p", "ready");
      ready.assign(std::string_view("worker is listening"));
      bus->publish(ready);

      for (int i = 0; i < 4; ++i) {
        while (propertyService.applyChangeNotifications() == 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        std::cout << "[worker] applied change #" << (i + 1) << ": maxSpeed = " << maxSpeed->getValue()
                  << ", message = '" << message->getValue() << "'\n";
      }
      return 0;
    }

    // main process: loads the property, later via HTTP
    ShmPropertyFixture shmFixture(propertyShmPath());
    auto changeBus = std::make_shared<lintel::EventBusView>(*bus);
    lintel::PropertyService propertyService({ shmFixture.repository() }, {});
    propertyService.setPropertyChangeBus(changeBus, "main");
    auto maxSpeed = propertyService.getOrCreate<int32_t>(
      "maxSpeed", "default", "MotorController", "main", "maximum speed of the motor controller", true, INITIAL_SPEED);
    auto message = propertyService.getOrCreate<std::string>(
      "message", "default", "MotorController", "main", "operator message", true, std::string(INITIAL_MESSAGE));

    if (!receivePolling(bus)) {
      std::cerr << "[main] worker never became ready\n";
      return 1;
    }

    for (const std::int32_t value : { 30, 40, 50 }) {
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
