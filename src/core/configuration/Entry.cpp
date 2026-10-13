#include "lintel/core/configuration/Entry.h"

namespace lintel {
Entry::Entry(std::string_view component) : m_component(component) {}

std::ostream &operator<<(std::ostream &os, const Entry &entry)
{
  os << "component: " << entry.m_component;
  return os;
}
}// namespace lintel
