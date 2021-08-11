#include "base_library/core/persistence/Arguments.h"

namespace db {
void Arguments::add(const Argument& argument) {
  m_arguments.push_back(argument);
}
std::size_t Arguments::getSize() const { return m_arguments.size(); }
Argument& Arguments::getArgument(std::size_t iterator) {
  return m_arguments.at(iterator);
}
Argument::Argument(const std::string& value, const std::string& name)
    : m_value(value), m_name(name) {}
const std::string& Argument::getValue() const { return m_value; }
const std::string& Argument::getName() const { return m_name; }
std::ostream& operator<<(std::ostream& os, const db::Argument& argument) {
  os << "m_value: " << argument.m_value << " m_name: " << argument.m_name;
  return os;
}
}  // namespace db