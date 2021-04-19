#ifndef LOGGING_STRINGIFYSERVICE_H
#define LOGGING_STRINGIFYSERVICE_H

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

#endif  // LOGGING_STRINGIFYSERVICE_H
