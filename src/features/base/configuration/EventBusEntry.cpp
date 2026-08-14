#include "base_library/features/base/configuration/EventBusEntry.h"

EventBusEntry::EventBusEntry(const std::string_view &component,
                             std::string _eventBusName, std::string _segmentName,
                             std::size_t _busCapacity,
                             std::size_t _subscriberCapacity,
                             std::size_t _maxTopics,
                             std::size_t _maxSubscribers)
        : Entry(component),
          eventBusName(std::move(_eventBusName)),
          segmentName(std::move(_segmentName)),
          busCapacity(_busCapacity),
          subscriberCapacity(_subscriberCapacity),
          maxTopics(_maxTopics),
          maxSubscribers(_maxSubscribers) {}

const std::string &EventBusEntry::get_name() const {
    return eventBusName;
}

const std::string &EventBusEntry::get_segment() const {
    return segmentName;
}

std::size_t EventBusEntry::get_bus_capacity() const {
    return busCapacity;
}

std::size_t EventBusEntry::get_subscriber_capacity() const {
    return subscriberCapacity;
}

std::size_t EventBusEntry::get_max_topics() const {
    return maxTopics;
}

std::size_t EventBusEntry::get_max_subscribers() const {
    return maxSubscribers;
}

std::ostream &operator<<(std::ostream &os, const EventBusEntry &entry) {
    os << static_cast<const Entry &>(entry)
       << " eventBusName: " << entry.eventBusName
       << " segmentName: " << entry.segmentName
       << " busCapacity: " << entry.busCapacity
       << " subscriberCapacity: " << entry.subscriberCapacity
       << " maxTopics: " << entry.maxTopics
       << " maxSubscribers: " << entry.maxSubscribers;
    return os;
}
