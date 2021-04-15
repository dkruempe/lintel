#ifndef CPP_BASE_LIBRARY_CONNECTIONTYPE_H
#define CPP_BASE_LIBRARY_CONNECTIONTYPE_H

#include <magic_enum.hpp>
#include <set>
#include <string>

namespace db {
class ConnectionType {
public:
  // value is defining priority of property repository type
  enum Value { UNDEFINED = -1, SQLite = 0, PostgreSQL };

  ConnectionType() = default;

  constexpr ConnectionType(Value value) : value(value) {}

  constexpr explicit ConnectionType(std::string_view enumName)
      : value(magic_enum::enum_cast<Value>(enumName).value_or(UNDEFINED)) {}

  operator Value() const { return value; }
  explicit operator bool() = delete;

  static std::set<Value> values() {
    auto values = magic_enum::enum_values<Value>();
    return std::set<Value>(values.begin(), values.end());
  }

  std::string toString() const {
    return std::string(magic_enum::enum_name<>(value));
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ConnectionType &connectionType) {
    os << magic_enum::enum_name<>(connectionType.value);
    return os;
  }

private:
  Value value;
};
} // namespace db

#endif // CPP_BASE_LIBRARY_CONNECTIONTYPE_H
