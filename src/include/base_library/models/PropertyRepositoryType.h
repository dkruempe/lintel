#ifndef PLC_PROPERTYREPOSITORYTYPE_H
#define PLC_PROPERTYREPOSITORYTYPE_H

#include <magic_enum.hpp>
#include <set>
#include <string>

class PropertyRepositoryType {
 public:
  // value is defining priority of property repository type
  enum Value { DEFAULT = -1, FILE_REPOSITORY = 0, SHM_REPOSITORY = 1 };

  PropertyRepositoryType() = default;

  constexpr PropertyRepositoryType(Value value) : value(value) {}

  constexpr explicit PropertyRepositoryType(std::string_view enumName)
      : value(magic_enum::enum_cast<Value>(enumName).value_or(DEFAULT)) {}

  operator Value() const { return value; }
  explicit operator bool() = delete;

  static std::set<Value> values() {
    auto values = magic_enum::enum_values<Value>();
    return std::set<Value>(values.begin(), values.end());
  }

  std::string toString() { return std::string(magic_enum::enum_name<>(value)); }

  friend std::ostream &operator<<(std::ostream &os,
                                  const PropertyRepositoryType &propertyRepositoryPriority) {
    os << magic_enum::enum_name<>(propertyRepositoryPriority.value);
    return os;
  }

 private:
  Value value;
};

#endif  // PLC_PROPERTYREPOSITORYTYPE_H
