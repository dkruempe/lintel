#ifndef LINTEL_EVENTBUSENTRY_H
#define LINTEL_EVENTBUSENTRY_H

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>

#include "lintel/core/configuration/Entry.h"
#include "lintel/features/base/events/EventBus.h"

namespace lintel {
/** The four independent capacities of an event bus. Grouping them into one
 *  aggregate keeps the constructor unambiguous (see EventBusEntry) and is the
 *  natural unit: all four are read from the same configuration block. */
struct EventBusCapacities
{
  /** Capacity of the point-to-point bus queue */
  std::size_t bus = 0;
  /** Capacity of every per-subscriber queue */
  std::size_t subscriber = 0;
  /** Maximum number of registerable topics */
  std::size_t topics = 0;
  /** Maximum number of registerable subscribers */
  std::size_t subscribers = 0;
};

/** Configuration entry for an event bus */
class EventBusEntry : public Entry
{
private:
  /** The name of the event bus */
  const std::string eventBusName;
  /** The name of the shared memory segment that stores the bus */
  const std::string segmentName;
  /** The capacity of the point-to-point bus queue */
  const std::size_t busCapacity;
  /** The capacity of every per-subscriber queue */
  const std::size_t subscriberCapacity;
  /** The maximum number of topics that can be registered */
  const std::size_t maxTopics;
  /** The maximum number of subscribers that can be registered */
  const std::size_t maxSubscribers;
  const DropPolicy pubPolicy;
  const bool sticky;
  const std::uint32_t stickyTtlMs;

public:
  /** Construct an event bus entry
   * @param component The configuration component name
   * @param eventBusName The bus name
   * @param segmentName The shared memory segment name
   * @param capacities The bus/subscriber/topic/subscriber limits */
  EventBusEntry(const std::string_view &component,
    std::string eventBusName,
    std::string segmentName,
    EventBusCapacities capacities,
    DropPolicy pubPolicy = DropPolicy::Fail,
    bool sticky = false,
    std::uint32_t stickyTtlMs = 60000);

  /** Get the event bus name
   * @return The bus name */
  [[nodiscard]] const std::string &getName() const;

  /** Get the shared memory segment name
   * @return The segment name */
  [[nodiscard]] const std::string &getSegment() const;

  /** Get the point-to-point queue capacity
   * @return The bus capacity */
  [[nodiscard]] constexpr std::size_t getBusCapacity() const { return busCapacity; }

  /** Get the per-subscriber queue capacity
   * @return The subscriber capacity */
  [[nodiscard]] constexpr std::size_t getSubscriberCapacity() const { return subscriberCapacity; }

  /** Get the maximum topic count
   * @return The maximum topics */
  [[nodiscard]] constexpr std::size_t getMaxTopics() const { return maxTopics; }

  /** Get the maximum subscriber count
   * @return The maximum subscribers */
  [[nodiscard]] constexpr std::size_t getMaxSubscribers() const { return maxSubscribers; }
  [[nodiscard]] constexpr DropPolicy getPubPolicy() const { return pubPolicy; }
  [[nodiscard]] constexpr bool getSticky() const { return sticky; }
  [[nodiscard]] constexpr std::uint32_t getStickyTtlMs() const { return stickyTtlMs; }

  /** Stream insertion operator
   * @param os The output stream
   * @param entry The entry to output
   * @return The output stream */
  friend std::ostream &operator<<(std::ostream &os, const EventBusEntry &entry);
};

}// namespace lintel
#endif// LINTEL_EVENTBUSENTRY_H
