#ifndef LOGGING_PROPERTYSERVICE_H
#define LOGGING_PROPERTYSERVICE_H

#include "exceptions/PropertyNotFoundException.h"
#include "models/Property.h"
#include "repositories/PropertyRepository.h"
#include "services/AbstractService.h"
#include <functional>
#include <map>
#include <vector>

#define DEFINE_PROPERTY(name, type, defaultValue)                              \
  std::shared_ptr<Property<type>> name =                                       \
      registerProperty<type>(#name, defaultValue)
#define LOAD_PROPERTIES() propertyService.getOrCreate(properties)

class PropertyService : public AbstractService<PropertyService> {
private:
  PropertyRepository &propertyRepository;
  // variables
  std::map<std::string, std::shared_ptr<PropertyBase>>
      properties; // identifier (name_instanceName_processName), Property
  // functions
  static std::string createIdentifier(const std::string &name,
                                      const std::string &instanceName,
                                      const std::string &className,
                                      const std::string &processName);
  static std::map<std::string, std::shared_ptr<PropertyBase>>
  init(const std::vector<std::shared_ptr<PropertyBase>>& properties);
  bool updateRepository = false;

public:
  PropertyService(PropertyRepository &propertyRepository,
                  const std::string &processName);
  void onInitialize() override;
  template <class T>
  std::shared_ptr<Property<T>>
  getOrCreate(const std::string &name, const std::string &instanceName,
              const std::string &className, const std::string &processName,
              const T &defaultValue = T());
  void
  getOrCreate(const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec);
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
    propertyRepository.save(property);
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
  propertyRepository.save(propertyBase);
}
#endif // LOGGING_PROPERTYSERVICE_H
