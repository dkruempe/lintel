#ifndef CPP_BASE_LIBRARY_POSTGRES_SERIALIZATION_H
#define CPP_BASE_LIBRARY_POSTGRES_SERIALIZATION_H

#include <date/tz.h>

#include <string>
#include <type_traits>

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

namespace postgresql {
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
#ifdef __APPLE__
IMPLEMENT_SERIALIZE(std::size_t, std::to_string, std::stoul);
#endif
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
IMPLEMENT_SERIALIZE(
    date::sys_time<std::chrono::microseconds>,
    [](const date::sys_time<std::chrono::microseconds> &time) -> std::string {
      return date::format("%Y-%m-%d %H:%M:%S%Ez", time);
    },
    [](const std::string &timeString)
        -> date::sys_time<std::chrono::microseconds> {
      std::stringstream ss(timeString);
      date::sys_time<std::chrono::microseconds> lt;
      ss >> date::parse("%Y-%m-%d %H:%M:%S%Ez", lt);
      return lt;
    })
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRES_SERIALIZATION_H
