#ifndef LOGGING_PROPERTYSERVICE_H
#define LOGGING_PROPERTYSERVICE_H

#include "exceptions/PropertyNotFoundException.h"
#include "models/Property.h"
#include "services/AbstractService.h"
#include <functional>
#include <map>
#include <vector>

#define DEFINE_PROPERTY(name, type, defaultValue)                              \
  std::shared_ptr<Property<type>> name = registerProperty<type>(#name, defaultValue);
#define LOAD_PROPERTIES() \
  propertyService.getOrCreate(properties);

class PropertyService : public AbstractService<PropertyService> {
private:
  // variables
  std::map<std::string, std::shared_ptr<PropertyBase>>
      properties; // identifier (name_instanceName_processName), Property
  // functions
  static std::string createIdentifier(const std::string &name,
                                      const std::string &instanceName,
                                      const std::string &className,
                                      const std::string &processName);

public:
  explicit PropertyService(const std::string &processName);
  template <class T>
  std::shared_ptr<Property<T>>
  getOrCreate(const std::string &name, const std::string &instanceName,
              const std::string &className, const std::string &processName,
              const T &defaultValue = T());
  void getOrCreate(const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec);
  std::shared_ptr<PropertyBase> get(const std::string &name,
                                    const std::string &instanceName,
                                    const std::string &className,
                                    const std::string &processName);
  std::vector<std::shared_ptr<PropertyBase>> allProperties();
  template <class T>
  void changeValueOf(const std::shared_ptr<PropertyBase> &property,
                     const T &value);
};
// template functions implementations
template <class T>
std::shared_ptr<Property<T>> PropertyService::getOrCreate(
    const std::string &name, const std::string &instanceName,
    const std::string &className, const std::string &processName,
    const T &defaultValue /* T() default Value */) {
  auto id = createIdentifier(name, instanceName, className, processName);
  try {
    auto propertyBase = get(name, instanceName, className, processName);
    return std::static_pointer_cast<Property<T>>(propertyBase);
  } catch (PropertyNotFoundException &exception) {
    auto property = std::make_shared<Property<T>>(name, instanceName, className,
                                                  processName, defaultValue);
    properties.insert({property->getIdentifier(), property});
    return property;
  }
}
template <class T>
void PropertyService::changeValueOf(
    const std::shared_ptr<PropertyBase> &property, const T &value) {
  std::shared_ptr<PropertyBase> propertyBase =
      get(property->getName(), property->getInstanceName(),
          property->getClassName(), property->getProcessName());
  std::static_pointer_cast<Property<T>>(propertyBase)->setValue(value);
}

#endif // LOGGING_PROPERTYSERVICE_H
