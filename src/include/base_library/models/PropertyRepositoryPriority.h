#ifndef PLC_PROPERTYREPOSITORYPRIORITY_H
#define PLC_PROPERTYREPOSITORYPRIORITY_H

#include <magic_enum.hpp>
#include <set>
#include <string>

class PropertyRepositoryPriority {
 public:
  enum Value { DEFAULT = -1, FILE_REPOSITORY = 0, SHM_REPOSITORY = 1 };

  PropertyRepositoryPriority() = default;

  constexpr PropertyRepositoryPriority(Value value) : value(value) {}

  constexpr explicit PropertyRepositoryPriority(std::string_view enumName)
      : value(magic_enum::enum_cast<Value>(enumName).value_or(DEFAULT)) {}

  operator Value() const { return value; }
  explicit operator bool() = delete;

  static std::set<Value> values() {
    auto values = magic_enum::enum_values<Value>();
    return std::set<Value>(values.begin(), values.end());
  }

  std::string toString() { return std::string(magic_enum::enum_name<>(value)); }

  friend std::ostream &operator<<(std::ostream &os,
                                  const PropertyRepositoryPriority &propertyRepositoryPriority) {
    os << magic_enum::enum_name<>(propertyRepositoryPriority.value);
    return os;
  }

 private:
  Value value;
};

#endif  // PLC_PROPERTYREPOSITORYPRIORITY_H
