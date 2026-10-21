#ifndef LINTEL_EVENTBUSDTOS_H
#define LINTEL_EVENTBUSDTOS_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/events/EventBus.h"

namespace lintel {
/** Runtime counters of an event bus, read while the bus is open. */
struct EventBusCounters
{
  /** Number of events dropped because a queue was full */
  std::uint64_t dropped = 0;
  /** Number of registered topics */
  std::size_t topics = 0;
  /** Number of reserved subscriber slots */
  std::size_t subscribers = 0;
  /** Best-effort depth of the point-to-point queue */
  std::size_t busDepth = 0;
  /** Last assigned sequence number */
  std::int64_t sequence = 0;
};

/**
 * DTO representing one configured event bus.
 *
 * The configuration is optional: a bus whose bootstrap configuration is
 * invalid is still listed, but without any configuration fields. The runtime
 * counters are only present while the bus could be opened in its shared memory
 * segment (AVAILABLE), because they have to be read from the bus itself.
 */
class EventBusDto : public JsonSerializable
{
private:
  /** The bus name */
  std::string m_name;
  /** The validated configuration, empty if the entry is invalid */
  std::optional<EventBusConfig> m_config;
  /** True if the bus could be opened in its shared memory segment */
  bool m_available = false;
  /** Number of events dropped because a queue was full */
  std::uint64_t m_dropped = 0;
  /** Number of registered topics */
  std::size_t m_topics = 0;
  /** Number of reserved subscriber slots */
  std::size_t m_subscribers = 0;
  /** Best-effort depth of the point-to-point queue */
  std::size_t m_busDepth = 0;
  /** Last assigned sequence number */
  std::int64_t m_sequence = 0;

public:
  /** Default constructor */
  EventBusDto() = default;

  /** Construct the DTO of a configured bus
   * @param name the bus name
   * @param config the validated bus configuration */
  EventBusDto(std::string name, const EventBusConfig &config);

  /** Construct the DTO of a bus without a usable configuration
   * @param name the bus name */
  explicit EventBusDto(std::string name);

  /** Store the runtime counters read from the bus.
   * @param counters the counters read while the bus was open */
  void markAvailable(const EventBusCounters &counters);

  /** @return the bus name */
  [[nodiscard]] const std::string &getName() const;

  /** @return the configuration, empty if the entry is invalid */
  [[nodiscard]] const std::optional<EventBusConfig> &getConfig() const;

  /** @return true if the bus could be opened */
  [[nodiscard]] bool isAvailable() const;

  /** @return the number of dropped events */
  [[nodiscard]] std::uint64_t getDropped() const;

  /** @return the number of registered topics */
  [[nodiscard]] std::size_t getTopics() const;

  /** @return the number of reserved subscriber slots */
  [[nodiscard]] std::size_t getSubscribers() const;

  /** @return the depth of the point-to-point queue */
  [[nodiscard]] std::size_t getBusDepth() const;

  /** @return the last assigned sequence number */
  [[nodiscard]] std::int64_t getSequence() const;

  using JsonSerializable::deserialize;

  /** Serialize to JSON
   * @param writer the rapidjson writer */
  void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

  /** Deserialize from JSON
   * @param obj the JSON value
   * @return true on success */
  bool deserialize(const rapidjson::Value &obj) override;
};

/** DTO representing the collection of all configured event buses */
class EventBusesDto : public JsonSerializable
{
private:
  /** The bus DTOs */
  std::vector<EventBusDto> m_eventBusDtos;

public:
  /** Default constructor */
  EventBusesDto() = default;

  /** Construct from existing bus DTOs
   * @param eventBusDtos the bus DTOs */
  explicit EventBusesDto(std::vector<EventBusDto> eventBusDtos);

  /** @return the bus DTOs */
  [[nodiscard]] const std::vector<EventBusDto> &getEventBusDtos() const;

  /** Serialize to JSON
   * @param writer the rapidjson writer */
  void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

  /** Deserialize from JSON string
   * @param json the JSON array to parse */
  void deserialize(const std::string &json) override;
};

}// namespace lintel
#endif// LINTEL_EVENTBUSDTOS_H
