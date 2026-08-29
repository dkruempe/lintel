#ifndef PROPERTY_H
#define PROPERTY_H

#include <chrono>
#include <memory>
#include <string>
#include <type_traits>

#include "base_library/core/services/StringifyService.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/property/models/PropertyBase.h"
#include "base_library/features/property/models/PropertyValueStorage.h"

class PropertyService;

/** Typed property with thread-safe, lock-free read/write access for POD types */
template<class T>
class Property : public PropertyBase {
private:
    PropertyValueStorage<T> m_storage;

    /** @return the type name string for this property's template type */
    static constexpr std::string_view getTypeName() {
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
    /** @param name property name
     *  @param instanceName instance name
     *  @param className class name
     *  @param processName process name
     *  @param value initial value
     *  @param description property description
     *  @param runtimeChange whether runtime changes are supported */
    Property(const std::string &name, const std::string &instanceName,
             const std::string &className, const std::string &processName,
             T value, const std::string &description, bool runtimeChange)
        : PropertyBase(name, instanceName, className, processName,
                       description, runtimeChange),
          m_storage(std::move(value)) {}

    Property()
        : PropertyBase("", "", "", "", "", false), m_storage() {}

    [[nodiscard]] std::string getType() const override {
        return std::string(getTypeName());
    }

    /** @return the current value (thread-safe read) */
    T getValue() { return m_storage.load(); }

    /** @return true if reads/writes are guaranteed lock-free */
    [[nodiscard]] static constexpr bool isLockFree() {
        return PropertyValueStorage<T>::isLockFree();
    }

    /** @return string representation of the current value */
    std::string toString() override {
        return StringifyService<T>::serializeToString(m_storage.load());
    }

    /** Set the value (thread-safe write) */
    void setValue(const T &value) { m_storage.store(value); }

    /** Set the value from a string representation */
    void setValueString(const std::string &value) override {
        m_storage.store(StringifyService<T>::deserializeFromString(value));
    }

    friend class PropertyService;
};

#endif  // PROPERTY_H
