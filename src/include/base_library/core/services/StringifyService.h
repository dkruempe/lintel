#ifndef LOGGING_STRINGIFYSERVICE_H
#define LOGGING_STRINGIFYSERVICE_H

#include <date/tz.h>

#include <string>

template <class T>
class StringifyService;

#define IMPLEMENT_STRINGIFY_SERVICE_FOR(type, convertToString, convertToValue) \
  template <>                                                                  \
  class StringifyService<type> {                                               \
   public:                                                                     \
    static std::string serializeToString(const type &value) {                  \
      return convertToString(value);                                           \
    }                                                                          \
    static type deserializeFromString(const std::string &string) {             \
      return convertToValue(string);                                           \
    }                                                                          \
  };

IMPLEMENT_STRINGIFY_SERVICE_FOR(int8_t, std::to_string, std::stoi)
IMPLEMENT_STRINGIFY_SERVICE_FOR(int16_t, std::to_string, std::stoi)
IMPLEMENT_STRINGIFY_SERVICE_FOR(int32_t, std::to_string, std::stoi)
IMPLEMENT_STRINGIFY_SERVICE_FOR(int64_t, std::to_string, std::stol)
IMPLEMENT_STRINGIFY_SERVICE_FOR(uint8_t, std::to_string, std::stoul)
IMPLEMENT_STRINGIFY_SERVICE_FOR(uint16_t, std::to_string, std::stoul)
IMPLEMENT_STRINGIFY_SERVICE_FOR(uint32_t, std::to_string, std::stoul)
IMPLEMENT_STRINGIFY_SERVICE_FOR(uint64_t, std::to_string, std::stoul)
IMPLEMENT_STRINGIFY_SERVICE_FOR(float, std::to_string, std::stof)
IMPLEMENT_STRINGIFY_SERVICE_FOR(double, std::to_string, std::stod)
IMPLEMENT_STRINGIFY_SERVICE_FOR(
    std::string,
    [](const std::string &string) -> std::string { return string; },
    [](const std::string &string) -> std::string { return string; })
IMPLEMENT_STRINGIFY_SERVICE_FOR(
    bool, [](bool value) -> std::string { return value ? "true" : "false"; },
    [](const std::string &string) -> bool {
      return string == "true" ? true : false;
    })
IMPLEMENT_STRINGIFY_SERVICE_FOR(
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
IMPLEMENT_STRINGIFY_SERVICE_FOR(
    std::chrono::seconds,
    [](const std::chrono::seconds &duration) -> std::string {
      return std::to_string(duration.count());
    },
    [](const std::string &duration) -> std::chrono::seconds {
      return std::chrono::seconds(std::stoull(duration));
    })
IMPLEMENT_STRINGIFY_SERVICE_FOR(
    std::chrono::minutes,
    [](const std::chrono::minutes &duration) -> std::string {
      return std::to_string(duration.count());
    },
    [](const std::string &duration) -> std::chrono::minutes {
      return std::chrono::minutes(std::stoull(duration));
    })

#endif  // LOGGING_STRINGIFYSERVICE_H
