#ifndef LINTEL_EVENTBUSSERVICE_H
#define LINTEL_EVENTBUSSERVICE_H

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/services/AbstractService.h"
#include "lintel/features/base/configuration/EventBusEntry.h"
#include "lintel/features/base/events/EventBus.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemoryService.h"

namespace lintel {
/**
 * Service that manages the lifecycle and access to EventBus instances stored
 * in shared memory segments. Consumers receive an IEventBus view that
 * forwards to the EventBus object in the segment.
 */
class EventBusService : public AbstractService<EventBusService>
{
private:
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<SharedMemoryService> m_sharedMemoryService;
  std::shared_ptr<SharedMemorySegmentManager> m_segmentManager;
  std::map<std::string, std::shared_ptr<EventBusEntry>> m_configurationMap;
  std::map<std::string, EventBus *> m_buses;
  std::map<std::string, std::size_t> m_generations;

  /** Collect the configured event bus entries.
   * @param configuration the application configuration
   * @return map of bus name to configuration entry */
  static std::map<std::string, std::shared_ptr<EventBusEntry>> init(
    const std::shared_ptr<Configuration> &configuration);

  /** Convert an entry to a runtime bus configuration.
   * @param entry the configuration entry
   * @return the validated bus configuration
   * @throws std::runtime_error if the configuration is invalid */
  static EventBusConfig configOfEntry(const std::shared_ptr<EventBusEntry> &entry);

  /** Look up a configured bus entry.
   * @param name the bus name
   * @return the configuration entry
   * @throws std::runtime_error if the bus is not configured */
  [[nodiscard]] const std::shared_ptr<EventBusEntry> &entryOf(const std::string &name) const;

  /** Resolve the EventBus of an entry; re-resolves after a segment remap.
   * @param entry the configuration entry
   * @return the EventBus stored in the shared memory segment */
  EventBus &resolve(const std::shared_ptr<EventBusEntry> &entry);

public:
  /** Construct an EventBusService.
   * @param configuration the application configuration
   * @param processName the current process name
   * @param sharedMemoryService the shared memory service
   * @param segmentManager the shared memory segment manager */
  EventBusService(const std::shared_ptr<Configuration> &configuration,
    std::shared_ptr<ProcessName> processName,
    std::shared_ptr<SharedMemoryService> sharedMemoryService,
    std::shared_ptr<SharedMemorySegmentManager> segmentManager);

  /** Get a view of the named event bus.
   * @param name the bus name
   * @return a view over the shared memory bus
   * @throws std::runtime_error if the bus is not configured */
  std::shared_ptr<IEventBus> of(const std::string &name);

  /** Names of all configured event buses.
   * @return the bus names in ascending order */
  [[nodiscard]] std::vector<std::string> namesOf() const;

  /** Runtime configuration of a configured event bus.
   *
   * The configuration is read from the bootstrap configuration and validated;
   * the bus itself is not resolved, so a bus whose segment is missing still
   * reports its configuration.
   * @param name the bus name
   * @return the validated bus configuration
   * @throws std::runtime_error if the bus is not configured or its configuration is invalid */
  [[nodiscard]] EventBusConfig configOf(const std::string &name) const;

  /** Best-effort depth of the point-to-point queue of a bus.
   *
   * IEventBus does not expose the queue depth, so the view is bypassed here.
   * @param name the bus name
   * @return the number of queued events
   * @throws std::runtime_error if the bus is not configured or cannot be resolved */
  [[nodiscard]] std::size_t busQueueDepthOf(const std::string &name);

  /** Best-effort depth of one subscriber queue of a bus.
   *
   * IEventBus does not expose the queue depth, so the view is bypassed here.
   * @param name the bus name
   * @param subscriber the subscriber name
   * @return the number of queued events, 0 for an unknown subscriber
   * @throws std::runtime_error if the bus is not configured or cannot be resolved */
  [[nodiscard]] std::size_t subscriberQueueDepthOf(const std::string &name, const std::string &subscriber);

  /** Construct all configured event buses in their segments. */
  void onInitialize() override;

  void onShutdown() override {}
};

}// namespace lintel
#endif// LINTEL_EVENTBUSSERVICE_H
