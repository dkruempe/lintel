#include "base_library/features/base/configuration/PropertyEntry.h"
std::shared_ptr<PropertyBase> &PropertyEntry::getProperty() { return property; }

PropertyEntry::PropertyEntry(std::string_view component,
                             std::shared_ptr<PropertyBase> property)
    : Entry(component), property(std::move(property)) {}

std::ostream &operator<<(std::ostream &os, const PropertyEntry &entry) {
  os << static_cast<const Entry &>(entry) << *entry.property;
  return os;
}
