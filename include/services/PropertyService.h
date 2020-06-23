#ifndef LOGGING_PROPERTYSERVICE_H
#define LOGGING_PROPERTYSERVICE_H

#include "models/Property.h"
#include <exceptions/PropertyNotFoundException.h>
#include <functional>
#include <map>
#include <utility>
#include <vector>

class PropertyService {
private:
  // variables
  std::map<std::string, std::shared_ptr<PropertyBase>>
      properties; // identifier (name_instanceName_processName), Property
  // functions
  static std::string createIdentifier(const std::string &name,
                                      const std::string &instanceName,
                                      const std::string &processName);

public:
  static bool Register(const std::string &type,
                       std::function<std::shared_ptr<PropertyBase>(
                           std::string, std::string,
                           std::string, std::string)>
                           createFunc) {
    auto it = GetMap().find(type);
    if (it == GetMap().end()) {
      GetMap()[type] = std::move(createFunc);
      return true;
    }
    return false;
  }

  static std::shared_ptr<PropertyBase> Create(const std::string &name,
                                              const std::string &instanceName,
                                              const std::string &processName,
                                              const std::string &type,
                                              const std::string &value) {
    auto it = GetMap().find(type);
    if (it != GetMap().end()) {
      return it->second(name, instanceName, processName, value);
    }

    return nullptr;
  }

  static std::map<std::string,
                  std::function<std::shared_ptr<PropertyBase>(
                      std::string, std::string, std::string, std::string)>> &
  GetMap() {
    static std::map<std::string,
                    std::function<std::shared_ptr<PropertyBase>(
                        std::string, std::string, std::string, std::string)>>
        s_methods;
    return s_methods;
  }
  template <class T>
  std::shared_ptr<Property<T>>
  getOrCreate(const std::string &name, const std::string &instanceName,
              const std::string &processName, const T &defaultValue = T());
  std::shared_ptr<PropertyBase> get(const std::string &name,
                                    const std::string &instanceName,
                                    const std::string &processName);
  std::vector<std::shared_ptr<PropertyBase>> allProperties();
  template <class T>
  void changeValueOf(const std::shared_ptr<PropertyBase> &property,
                     const T &value);
};
// template functions implementations
template <class T>
std::shared_ptr<Property<T>>
PropertyService::getOrCreate(const std::string &name,
                             const std::string &instanceName,
                             const std::string &processName,
                             const T &defaultValue /* T() default Value */) {
  auto id = createIdentifier(name, instanceName, processName);
  try {
    auto propertyBase = get(name, instanceName, processName);
    return std::static_pointer_cast<Property<T>>(propertyBase);
  } catch (PropertyNotFoundException &exception) {
    auto property = std::make_shared<Property<T>>(
        name, instanceName, processName, defaultValue);
    properties.insert({property->getIdentifier(), property});
    return property;
  }
}
template <class T>
void PropertyService::changeValueOf(
    const std::shared_ptr<PropertyBase> &property, const T &value) {
  std::shared_ptr<PropertyBase> propertyBase =
      get(property->getName(), property->getInstanceName(),
          property->getProcessName());
  std::static_pointer_cast<Property<T>>(propertyBase)->setValue(value);
}

#endif // LOGGING_PROPERTYSERVICE_H
