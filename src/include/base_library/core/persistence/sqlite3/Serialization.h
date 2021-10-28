#ifndef CPP_BASE_LIBRARY_SQLITE_SERIALIZATION_H
#define CPP_BASE_LIBRARY_SQLITE_SERIALIZATION_H

#include <exception>
#include <memory>

#include "base_library/core/persistence/postgresql/Serialization.h"
#include "base_library/core/persistence/sqlite3/Serialization.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

#define IMPLEMENT_SERIALIZE(type, convertToString, convertToValue) \
  template <>                                                      \
  class Serialization<type> {                                      \
   public:                                                         \
    static std::string serialize(const type &value) {              \
      return convertToString(value);                               \
    }                                                              \
    static type deserialize(const std::string &value) {            \
      return convertToValue(value);                                \
    }                                                              \
  };

namespace sqlite {
template <typename T>
class Serialization {};
IMPLEMENT_SERIALIZE(int8_t, std::to_string, std::stoi)
IMPLEMENT_SERIALIZE(int16_t, std::to_string, std::stoi)
IMPLEMENT_SERIALIZE(int32_t, std::to_string, std::stoi)
IMPLEMENT_SERIALIZE(int64_t, std::to_string, std::stol)
IMPLEMENT_SERIALIZE(uint8_t, std::to_string, std::stoul)
IMPLEMENT_SERIALIZE(uint16_t, std::to_string, std::stoul)
IMPLEMENT_SERIALIZE(uint32_t, std::to_string, std::stoul)
IMPLEMENT_SERIALIZE(uint64_t, std::to_string, std::stoul)
IMPLEMENT_SERIALIZE(std::size_t, std::to_string, std::stoul);
IMPLEMENT_SERIALIZE(float, std::to_string, std::stof)
IMPLEMENT_SERIALIZE(double, std::to_string, std::stod)
IMPLEMENT_SERIALIZE(
    std::string,
    [](const std::string &string) -> std::string { return string; },
    [](const std::string &string) -> std::string { return string; })
IMPLEMENT_SERIALIZE(
    bool, [](bool value) -> std::string { return value ? "t" : "f"; },
    [](const std::string &string) -> bool {
      return string == "t" ? true : false;
    })
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_SERIALIZATION_H
