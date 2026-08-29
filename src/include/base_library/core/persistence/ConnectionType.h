#ifndef CPP_BASE_LIBRARY_CONNECTIONTYPE_H
#define CPP_BASE_LIBRARY_CONNECTIONTYPE_H

#include <magic_enum/magic_enum.hpp>
#include <set>
#include <string>

namespace db {
/**
 * Defines the supported database connection types.
 */
class ConnectionType
{
public:
  // value is defining priority of property repository type
  enum Value { UNDEFINED = -1, SQLite = 0, PostgreSQL };

  /** Default constructor, initializes to UNDEFINED. */
  ConnectionType() = default;

  /**
   * Constructs from a Value enum.
   * @param value the connection type value
   */
  constexpr ConnectionType(Value value) : m_value(value) {}

  /**
   * Constructs from a string representation of a Value.
   * @param enumName string matching a Value name, defaults to UNDEFINED if not found
   */
  constexpr explicit ConnectionType(std::string_view enumName)
    : m_value(magic_enum::enum_cast<Value>(enumName).value_or(UNDEFINED))
  {}

  /** Implicit conversion to Value. */
  constexpr operator Value() const { return m_value; }

  explicit operator bool() = delete;

  /** @return set of all defined Value entries */
  static std::set<Value> values()
  {
    auto values = magic_enum::enum_values<Value>();
    return std::set<Value>(values.begin(), values.end());
  }

  /** @return string representation of the current value */
  std::string toString() const { return std::string(magic_enum::enum_name<>(m_value)); }

  /** Streams the string representation of the connection type. */
  friend std::ostream &operator<<(std::ostream &os, const ConnectionType &connectionType)
  {
    os << magic_enum::enum_name<>(connectionType.m_value);
    return os;
  }

private:
  Value m_value;
};
}// namespace db

#endif// CPP_BASE_LIBRARY_CONNECTIONTYPE_H
