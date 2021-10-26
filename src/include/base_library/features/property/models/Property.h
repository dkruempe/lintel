#ifndef PROPERTY_H
#define PROPERTY_H

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

#include "base_library/core/services/StringifyService.h"
#include "base_library/core/utils/TypeName.h"
#include "base_library/features/property/factories/PropertyFactory.h"
#include "base_library/features/property/models/PropertyBase.h"

class PropertyService;

template <class T>
class Property;

#define IMPLEMENT_PROPERTY(type, convertToString, convertToValue)              \
  template <>                                                                  \
  class Property<type> : public PropertyBase {                                 \
   private:                                                                    \
    type m_value;                                                              \
    std::shared_mutex m_mutex;                                                 \
    static bool Registration() {                                               \
      PropertyFactory::TCreateMethod func =                                    \
          [&](const std::string &name, const std::string &instanceName,        \
              const std::string &className, const std::string &processName,    \
              const std::string &value, const std::string &description,        \
              bool runtime) -> std::shared_ptr<PropertyBase> {                 \
        return std::make_shared<Property<type>>(                               \
            name, instanceName, className, processName,                        \
            StringifyService<type>::deserializeFromString(value), description, \
            runtime);                                                          \
      };                                                                       \
      return PropertyFactory::Register(#type, func);                           \
    }                                                                          \
    static bool registered;                                                    \
                                                                               \
    void setValue(const type &value) {                                         \
      std::unique_lock<std::shared_mutex> lock(m_mutex);                       \
      m_value = value;                                                         \
    }                                                                          \
                                                                               \
    void setValueString(const std::string &value) override {                   \
      std::unique_lock<std::shared_mutex> lock(m_mutex);                       \
      m_value = convertToValue(value);                                         \
    }                                                                          \
                                                                               \
   public:                                                                     \
    Property(const std::string &name, const std::string &instanceName,         \
             const std::string &className, const std::string &processName,     \
             type value, const std::string &description, bool runtimeChange)   \
        : PropertyBase(name, instanceName, className, processName,             \
                       description, runtimeChange),                            \
          m_value(std::move(value)) {}                                         \
    [[nodiscard]] std::string getType() const override { return #type; }       \
    type getValue() {                                                          \
      std::shared_lock<std::shared_mutex> lock(m_mutex);                       \
      return m_value;                                                          \
    }                                                                          \
    std::string toString() override {                                          \
      std::shared_lock<std::shared_mutex> lock(m_mutex);                       \
      return convertToString(m_value);                                         \
    }                                                                          \
    friend class PropertyService;                                              \
  };
IMPLEMENT_PROPERTY(int8_t, StringifyService<int8_t>::serializeToString,
                   StringifyService<int8_t>::deserializeFromString)
IMPLEMENT_PROPERTY(int16_t, StringifyService<int16_t>::serializeToString,
                   StringifyService<int16_t>::deserializeFromString)
IMPLEMENT_PROPERTY(int32_t, StringifyService<int32_t>::serializeToString,
                   StringifyService<int32_t>::deserializeFromString)
IMPLEMENT_PROPERTY(int64_t, StringifyService<int64_t>::serializeToString,
                   StringifyService<int64_t>::deserializeFromString)
IMPLEMENT_PROPERTY(uint8_t, StringifyService<uint8_t>::serializeToString,
                   StringifyService<uint8_t>::deserializeFromString)
IMPLEMENT_PROPERTY(uint16_t, StringifyService<uint16_t>::serializeToString,
                   StringifyService<uint16_t>::deserializeFromString)
IMPLEMENT_PROPERTY(uint32_t, StringifyService<uint32_t>::serializeToString,
                   StringifyService<uint32_t>::deserializeFromString)
IMPLEMENT_PROPERTY(uint64_t, StringifyService<uint64_t>::serializeToString,
                   StringifyService<uint64_t>::deserializeFromString)
IMPLEMENT_PROPERTY(std::size_t, StringifyService<std::size_t>::serializeToString,
                   StringifyService<std::size_t>::deserializeFromString)
IMPLEMENT_PROPERTY(float, StringifyService<float>::serializeToString,
                   StringifyService<float>::deserializeFromString)
IMPLEMENT_PROPERTY(double, StringifyService<double>::serializeToString,
                   StringifyService<double>::deserializeFromString)
IMPLEMENT_PROPERTY(std::string,
                   StringifyService<std::string>::serializeToString,
                   StringifyService<std::string>::deserializeFromString)
IMPLEMENT_PROPERTY(bool, StringifyService<bool>::serializeToString,
                   StringifyService<bool>::deserializeFromString)
IMPLEMENT_PROPERTY(
    std::chrono::seconds,
    StringifyService<std::chrono::seconds>::serializeToString,
    StringifyService<std::chrono::seconds>::deserializeFromString)
IMPLEMENT_PROPERTY(
    std::chrono::minutes,
    StringifyService<std::chrono::minutes>::serializeToString,
    StringifyService<std::chrono::minutes>::deserializeFromString)

#endif /* PROPERTY_H */
