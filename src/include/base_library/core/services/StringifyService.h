#ifndef LOGGING_STRINGIFYSERVICE_H
#define LOGGING_STRINGIFYSERVICE_H

#include <date/tz.h>

#include <chrono>
#include <sstream>
#include <string>
#include <type_traits>

template<class T>
class StringifyService {
public:
    static std::string serializeToString(const T &value) {
        if constexpr (std::is_same_v<T, std::string>) {
            return value;
        } else if constexpr (std::is_same_v<T, bool>) {
            return value ? "true" : "false";
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(value);
        } else if constexpr (std::is_same_v<T, date::sys_time<std::chrono::microseconds>>) {
            return date::format("%Y-%m-%d %H:%M:%S%Ez", value);
        } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>
                          || std::is_same_v<T, std::chrono::seconds>
                          || std::is_same_v<T, std::chrono::minutes>) {
            return std::to_string(value.count());
        }
    }

    static T deserializeFromString(const std::string &string) {
        if constexpr (std::is_same_v<T, std::string>) {
            return string;
        } else if constexpr (std::is_same_v<T, bool>) {
            return string == "true";
        } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
            return static_cast<T>(std::stoll(string));
        } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
            return static_cast<T>(std::stoull(string));
        } else if constexpr (std::is_same_v<T, float>) {
            return std::stof(string);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(string);
        } else if constexpr (std::is_same_v<T, date::sys_time<std::chrono::microseconds>>) {
            std::stringstream ss(string);
            date::sys_time<std::chrono::microseconds> lt;
            ss >> date::parse("%Y-%m-%d %H:%M:%S%Ez", lt);
            return lt;
        } else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) {
            return std::chrono::milliseconds(std::stoull(string));
        } else if constexpr (std::is_same_v<T, std::chrono::seconds>) {
            return std::chrono::seconds(std::stoull(string));
        } else if constexpr (std::is_same_v<T, std::chrono::minutes>) {
            return std::chrono::minutes(std::stoull(string));
        }
    }
};

#endif  // LOGGING_STRINGIFYSERVICE_H
