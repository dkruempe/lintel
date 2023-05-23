#include "base_library/features/base/configuration/Entry.h"

std::string_view Entry::getConfigurationParserComponent() {
    return m_component;
}

Entry::Entry(std::string_view component) : m_component(component) {}

std::ostream &operator<<(std::ostream &os, const Entry &entry) {
    os << "component: " << entry.m_component;
    return os;
}
