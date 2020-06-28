#include "services/PropertyService.h"
#include "exceptions/PropertyNotFoundException.h"
#include <algorithm>
PropertyService::PropertyService(const std::string &processName)
    : AbstractService(processName, "PropertyService") {}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allProperties() {
  std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
  std::transform(properties.begin(), properties.end(),
                 std::back_inserter(propertiesVector),
                 [](const auto &iter) { return iter.second; });
  return propertiesVector;
}
std::string PropertyService::createIdentifier(const std::string &name,
                                              const std::string &instanceName,
                                              const std::string &className,
                                              const std::string &processName) {
  return name + "_" + instanceName + "_" + className + "_" + processName;
}
std::shared_ptr<PropertyBase>
PropertyService::get(const std::string &name, const std::string &instanceName,
                     const std::string &className,
                     const std::string &processName) {
  const std::string &identifier =
      createIdentifier(name, instanceName, className, processName);
  auto found = properties.find(identifier);
  if (found == properties.end()) {
    throw PropertyNotFoundException(name, instanceName, className, processName);
  }
  return found->second;
}
void PropertyService::getOrCreate(const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec) {
  for (std::shared_ptr<PropertyBase> property : propertiesVec) {
    try {
      auto newProperty= get(property->getName(), property->getInstanceName(),
                     property->getClassName(), property->getProcessName());
      property = newProperty;
    } catch (PropertyNotFoundException &exception) {
      properties.insert({property->getIdentifier(), property});
    }
  }
}