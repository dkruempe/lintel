#include "lintel/features/property/configuration/PropertyEntry.h"

std::shared_ptr<PropertyBase> &PropertyEntry::getProperty() {
    return m_property;
}

PropertyEntry::PropertyEntry(std::string_view component,
                             std::shared_ptr<PropertyBase> property)
        : Entry(component), m_property(std::move(property)) {}

std::ostream &operator<<(std::ostream &os, const PropertyEntry &entry) {
    os << static_cast<const Entry &>(entry) << *entry.m_property;
    return os;
}
