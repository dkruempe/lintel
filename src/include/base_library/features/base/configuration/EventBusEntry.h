#ifndef CPP_BASE_LIBRARY_EVENTBUSENTRY_H
#define CPP_BASE_LIBRARY_EVENTBUSENTRY_H

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>

#include "base_library/features/base/configuration/Entry.h"

/** Configuration entry for an event bus */
class EventBusEntry : public Entry {
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

public:
    /** Construct an event bus entry
     * @param component The configuration component name
     * @param eventBusName The bus name
     * @param segmentName The shared memory segment name
     * @param busCapacity The point-to-point queue capacity
     * @param subscriberCapacity The per-subscriber queue capacity
     * @param maxTopics The maximum topic count
     * @param maxSubscribers The maximum subscriber count */
    EventBusEntry(const std::string_view &component, std::string eventBusName,
                  std::string segmentName, std::size_t busCapacity,
                  std::size_t subscriberCapacity, std::size_t maxTopics,
                  std::size_t maxSubscribers);

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

    /** Stream insertion operator
     * @param os The output stream
     * @param entry The entry to output
     * @return The output stream */
    friend std::ostream &operator<<(std::ostream &os,
                                    const EventBusEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_EVENTBUSENTRY_H
