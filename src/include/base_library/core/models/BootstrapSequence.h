#ifndef CPP_BASE_LIBRARY_BOOTSTRAP_H
#define CPP_BASE_LIBRARY_BOOTSTRAP_H

#include <magic_enum.hpp>
#include <set>
#include <string>

class BootstrapSequence {
 public:
  // value is defining priority of property repository type
  enum Value { Undefined = 0, Database = 1, VirtualGroups = 2 };

  BootstrapSequence() = default;

  constexpr BootstrapSequence(Value value) : m_value(value) {}

  constexpr explicit BootstrapSequence(std::string_view enumName)
      : m_value(magic_enum::enum_cast<Value>(enumName).value_or(Undefined)) {}

  operator Value() const { return m_value; }
  explicit operator bool() = delete;

  static std::set<Value> values() {
    auto values = magic_enum::enum_values<Value>();
    return std::set<Value>(values.begin(), values.end());
  }

  std::string toString() const {
    return std::string(magic_enum::enum_name<>(m_value));
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const BootstrapSequence &bootstrap) {
    os << magic_enum::enum_name<>(bootstrap.m_value);
    return os;
  }

 private:
  Value m_value;
};

#endif  // CPP_BASE_LIBRARY_BOOTSTRAP_H
