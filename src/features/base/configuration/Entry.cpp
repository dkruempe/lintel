#include "base_library/features/base/configuration/Entry.h"
std::string_view Entry::getConfigurationParserComponent() { return component; }
Entry::Entry(std::string_view component) : component(component) {}
std::ostream &operator<<(std::ostream &os, const Entry &entry) {
  os << "component: " << entry.component;
  return os;
}
