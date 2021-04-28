#ifndef LOGGING_PROPERTYSERVICE_H
#define LOGGING_PROPERTYSERVICE_H

#include <functional>
#include <map>
#include <ostream>
#include <vector>

#include "PropertyService.h"
#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/property/exceptions/PropertyNoRuntimeChangeSupported.h"
#include "base_library/features/property/exceptions/PropertyNotFoundException.h"
#include "base_library/features/property/models/Property.h"
#include "base_library/features/property/repositories/PropertyRepository.h"

#define DEFINE_PROPERTY(name, type, defaultValue, description, runtime)     \
  std::shared_ptr<Property<type>> name =                                    \
      registerProperty<type>(std::string(#name), defaultValue, description, \
                             runtime, __FILE__, __LINE__)
#define LOAD_PROPERTIES()                     \
  if (propertyService != nullptr) {           \
    propertyService->getOrCreate(properties); \
  }

class PropertyService {
 private:
  std::shared_ptr<PropertyRepository> propertyRepository;
  // variables
  std::map<std::string, std::shared_ptr<PropertyBase>>
      properties;  // identifier (name_instanceName_processName), Property
  // functions
  static std::string createIdentifier(const std::string &name,
                                      const std::string &instanceName,
                                      const std::string &className,
                                      const std::string &processName);
  static std::map<std::string, std::shared_ptr<PropertyBase>> init(
      const std::vector<std::shared_ptr<PropertyRepository>> &repoProperties);
  static std::shared_ptr<PropertyRepository> searchRuntimeRepository(
      const std::vector<std::shared_ptr<PropertyRepository>>
          &propertyRepository);

 public:
  PropertyService(const std::vector<std::shared_ptr<PropertyRepository>>
                      &propertyRepositories,
                  const std::vector<std::shared_ptr<AbstractServiceInterface>>
                      &abstractServices);

  template <class T>
  std::shared_ptr<Property<T>> getOrCreate(const std::string &name,
                                           const std::string &instanceName,
                                           const std::string &className,
                                           const std::string &processName,
                                           const std::string &description,
                                           bool runtimeChange,
                                           const T &defaultValue = T());
  void getOrCreate(
      const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec);
  std::shared_ptr<PropertyBase> &get(const std::string &name,
                                     const std::string &instanceName,
                                     const std::string &className,
                                     const std::string &processName);
  std::vector<std::shared_ptr<PropertyBase>> allProperties();
  template <class T>
  void changeValueOf(const std::shared_ptr<PropertyBase> &property,
                     const T &value);

  void changeStringValueOf(const std::shared_ptr<PropertyBase> &property,
                           const std::string &value);
};
// template functions implementations
template <class T>
std::shared_ptr<Property<T>> PropertyService::getOrCreate(
    const std::string &name, const std::string &instanceName,
    const std::string &className, const std::string &processName,
    const std::string &description, bool runtimeChange,
    const T &defaultValue /* T() default Value */) {
  auto id = createIdentifier(name, instanceName, className, processName);
  try {
    auto propertyBase = get(name, instanceName, className, processName);
    return std::static_pointer_cast<Property<T>>(propertyBase);
  } catch (PropertyNotFoundException &exception) {
    auto property = std::make_shared<Property<T>>(name, instanceName, className,
                                                  processName, defaultValue,
                                                  description, runtimeChange);
    properties.insert({property->getIdentifier(), property});
    if (propertyRepository != nullptr) {
      propertyRepository->save(property);
    }
    return property;
  }
}
template <class T>
void PropertyService::changeValueOf(
    const std::shared_ptr<PropertyBase> &property, const T &value) {
  std::shared_ptr<PropertyBase> propertyBase =
      get(property->getName(), property->getInstanceName(),
          property->getClassName(), property->getProcessName());
  if (!propertyBase->isRuntimeChange()) {
    throw PropertyNoRuntimeChangeSupported(propertyBase);
  }
  std::static_pointer_cast<Property<T>>(propertyBase)->setValue(value);
  std::static_pointer_cast<Property<T>>(propertyBase)
      ->setDataStorage(propertyRepository->getDataStorage());
  if (propertyRepository != nullptr) {
    propertyRepository->save(propertyBase);
  }
}

#endif  // LOGGING_PROPERTYSERVICE_H
