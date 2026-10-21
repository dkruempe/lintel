#include "lintel/features/base/configuration/EventBusEntry.h"

namespace lintel {
EventBusEntry::EventBusEntry(const std::string_view &component,
  std::string _eventBusName,
  std::string _segmentName,
  EventBusCapacities capacities,
  DropPolicy _pubPolicy,
  bool _sticky,
  std::uint32_t _stickyTtlMs)
  : Entry(component), eventBusName(std::move(_eventBusName)), segmentName(std::move(_segmentName)),
    busCapacity(capacities.bus), subscriberCapacity(capacities.subscriber), maxTopics(capacities.topics),
    maxSubscribers(capacities.subscribers), pubPolicy(_pubPolicy), sticky(_sticky), stickyTtlMs(_stickyTtlMs)
{}

const std::string &EventBusEntry::getName() const { return eventBusName; }

const std::string &EventBusEntry::getSegment() const { return segmentName; }

std::ostream &operator<<(std::ostream &os, const EventBusEntry &entry)
{
  os << static_cast<const Entry &>(entry) << " eventBusName: " << entry.eventBusName
     << " segmentName: " << entry.segmentName << " busCapacity: " << entry.busCapacity
     << " subscriberCapacity: " << entry.subscriberCapacity << " maxTopics: " << entry.maxTopics
     << " maxSubscribers: " << entry.maxSubscribers;
  return os;
}
}// namespace lintel
