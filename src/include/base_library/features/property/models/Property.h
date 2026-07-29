#ifndef PROPERTY_H
#define PROPERTY_H

#include <chrono>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>

#include "base_library/core/services/StringifyService.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/property/models/PropertyBase.h"

class PropertyService;

template<class T>
class Property : public PropertyBase {
private:
    T m_value;
    std::shared_mutex m_mutex;

    static std::string_view getTypeName() {
        if constexpr (std::is_same_v<T, int8_t>) return "int8_t";
        else if constexpr (std::is_same_v<T, int16_t>) return "int16_t";
        else if constexpr (std::is_same_v<T, int32_t>) return "int32_t";
        else if constexpr (std::is_same_v<T, int64_t>) return "int64_t";
        else if constexpr (std::is_same_v<T, uint8_t>) return "uint8_t";
        else if constexpr (std::is_same_v<T, uint16_t>) return "uint16_t";
        else if constexpr (std::is_same_v<T, uint32_t>) return "uint32_t";
        else if constexpr (std::is_same_v<T, uint64_t>) return "uint64_t";
        else if constexpr (std::is_same_v<T, std::string>) return "std::string";
        else if constexpr (std::is_same_v<T, std::chrono::milliseconds>) return "std::chrono::milliseconds";
        else if constexpr (std::is_same_v<T, std::chrono::seconds>) return "std::chrono::seconds";
        else if constexpr (std::is_same_v<T, std::chrono::minutes>) return "std::chrono::minutes";
        else return type_name<T>();
    }

public:
    Property(const std::string &name, const std::string &instanceName,
             const std::string &className, const std::string &processName,
             T value, const std::string &description, bool runtimeChange)
        : PropertyBase(name, instanceName, className, processName,
                       description, runtimeChange),
          m_value(std::move(value)) {}

    Property()
        : PropertyBase("", "", "", "", "", false), m_value{} {}

    [[nodiscard]] std::string getType() const override {
        return std::string(getTypeName());
    }

    T getValue() {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_value;
    }

    std::string toString() override {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return StringifyService<T>::serializeToString(m_value);
    }

    void setValue(const T &value) {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        m_value = value;
    }

    void setValueString(const std::string &value) override {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        m_value = StringifyService<T>::deserializeFromString(value);
    }

    friend class PropertyService;
};

#endif  // PROPERTY_H
