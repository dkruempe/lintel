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

const std::string &EventBusEntry::getName() const {
    return eventBusName;
}

const std::string &EventBusEntry::getSegment() const {
    return segmentName;
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
