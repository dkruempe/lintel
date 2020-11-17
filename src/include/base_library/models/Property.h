#ifndef PROPERTY_H
#define PROPERTY_H

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

#include "PropertyBase.h"
#include "base_library/factories/PropertyFactory.h"
#include "base_library/services/StringifyService.h"
#include "base_library/utils/TypeName.h"

class PropertyService;

template <class T>
class Property;

#define IMPLEMENT_PROPERTY(type, convertToString, convertToValue)           \
  template <>                                                               \
  class Property<type> : public PropertyBase {                              \
   private:                                                                 \
    type value;                                                             \
    std::shared_mutex mutex;                                                \
    static bool Registration() {                                            \
      PropertyFactory::TCreateMethod func =                                 \
          [&](const std::string &name, const std::string &instanceName,     \
              const std::string &className, const std::string &processName, \
              const std::string &value) -> std::shared_ptr<PropertyBase> {  \
        return std::make_shared<Property<type>>(                            \
            name, instanceName, className, processName,                     \
            StringifyService<type>::deserializeFromString(value));          \
      };                                                                    \
      return PropertyFactory::Register(#type, func);                        \
    }                                                                       \
    static bool registered;                                                 \
                                                                            \
    void setValue(const type &value) {                                      \
      std::unique_lock<std::shared_mutex> lock(mutex);                      \
      this->value = value;                                                  \
    }                                                                       \
                                                                            \
    void setValueString(const std::string &value) override {                \
      std::unique_lock<std::shared_mutex> lock(mutex);                      \
      this->value = convertToValue(value);                                  \
    }                                                                       \
                                                                            \
   public:                                                                  \
    Property(const std::string &name, const std::string &instanceName,      \
             const std::string &className, const std::string &processName,  \
             type value)                                                    \
        : PropertyBase(name, instanceName, className, processName),         \
          value(std::move(value)) {}                                        \
    [[nodiscard]] std::string getType() const override { return #type; }    \
    type getValue() {                                                       \
      std::shared_lock<std::shared_mutex> lock(mutex);                      \
      return value;                                                         \
    }                                                                       \
    std::string toString() override {                                       \
      std::shared_lock<std::shared_mutex> lock(mutex);                      \
      return convertToString(value);                                        \
    }                                                                       \
    friend class PropertyService;                                           \
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
IMPLEMENT_PROPERTY(float, StringifyService<float>::serializeToString,
                   StringifyService<float>::deserializeFromString)
IMPLEMENT_PROPERTY(double, StringifyService<double>::serializeToString,
                   StringifyService<double>::deserializeFromString)
IMPLEMENT_PROPERTY(std::string,
                   StringifyService<std::string>::serializeToString,
                   StringifyService<std::string>::deserializeFromString)
IMPLEMENT_PROPERTY(bool, StringifyService<bool>::serializeToString,
                   StringifyService<bool>::deserializeFromString)

#endif /* PROPERTY_H */
