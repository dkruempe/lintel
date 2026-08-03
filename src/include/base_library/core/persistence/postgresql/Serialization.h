#ifndef CPP_BASE_LIBRARY_POSTGRES_SERIALIZATION_H
#define CPP_BASE_LIBRARY_POSTGRES_SERIALIZATION_H

#include <date/tz.h>

#include <string>
#include <type_traits>

/**
 * Macro that generates a full specialization of Serialization for a given type
 * with the provided serialization and deserialization lambdas or functions.
 * @param type the type to specialize for
 * @param convertToString expression/lambda to convert the type to std::string
 * @param convertToValue expression/lambda to convert std::string to the type
 */
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
    /**
     * Primary template for PostgreSQL serialization.
     * Full specializations are provided for all supported types.
     * @tparam T the type to serialize/deserialize
     * @tparam Enable SFINAE parameter for partial specializations
     */
    template<typename T, typename Enable = void>
    class Serialization {
    };

    IMPLEMENT_SERIALIZE(int8_t, std::to_string, std::stoi)

    IMPLEMENT_SERIALIZE(int16_t, std::to_string, std::stoi)

    IMPLEMENT_SERIALIZE(int32_t, std::to_string, std::stoi)

    IMPLEMENT_SERIALIZE(int64_t, std::to_string, std::stol)

    IMPLEMENT_SERIALIZE(uint8_t, std::to_string, std::stoul)

    IMPLEMENT_SERIALIZE(uint16_t, std::to_string, std::stoul)

    IMPLEMENT_SERIALIZE(uint32_t, std::to_string, std::stoul)

    IMPLEMENT_SERIALIZE(uint64_t, std::to_string, std::stoul)

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
                std::string createdTimestamp = date::format("%Y-%m-%d %H:%M:%S%Ez", time);
                std::replace(createdTimestamp.begin(), createdTimestamp.end(), ',', '.');
                return createdTimestamp;
            },
            [](const std::string &timeString)
                    -> date::sys_time<std::chrono::microseconds> {
                std::stringstream ss(timeString);
                date::sys_time<std::chrono::microseconds> lt;
                ss >> std::chrono::parse("%Y-%m-%d %H:%M:%S%Ez", lt);
                return lt;
             })

    // Only activated when std::size_t is distinct from all fixed-width integer types
    // (full specializations above take precedence over this partial specialization)
    template<typename T>
    class Serialization<T, std::enable_if_t<std::is_same_v<T, std::size_t>>> {
    public:
        static std::string serialize(const T& value) {
            return std::to_string(value);
        }
        static T deserialize(const std::string& value) {
            return static_cast<T>(std::stoul(value));
        }
    };
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRES_SERIALIZATION_H
