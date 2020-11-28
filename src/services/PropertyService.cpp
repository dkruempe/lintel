#include "base_library/services/PropertyService.h"

#include <algorithm>

#include "base_library/exceptions/PropertyNotFoundException.h"

std::map<std::string, std::shared_ptr<PropertyBase>> PropertyService::init(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {
  std::map<std::string, std::shared_ptr<PropertyBase>> propertiesMap;
  for (const auto &property : properties) {
    propertiesMap.insert({property->getIdentifier(), property});
  }
  return propertiesMap;
}
PropertyService::PropertyService(
    const std::shared_ptr<PropertyRepository> &propertyRepository,
    const std::shared_ptr<ProcessName> &processName)
    : AbstractService(processName->getProcessName(), "PropertyService"),
      propertyRepository(propertyRepository),
      properties(init(propertyRepository->awake())) {}
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
std::shared_ptr<PropertyBase> &PropertyService::get(
    const std::string &name, const std::string &instanceName,
    const std::string &className, const std::string &processName) {
  const std::string &identifier =
      createIdentifier(name, instanceName, className, processName);
  auto found = properties.find(identifier);
  if (found == properties.end()) {
    throw PropertyNotFoundException(name, instanceName, className, processName);
  }
  return found->second;
}
void PropertyService::getOrCreate(
    const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec) {
  for (const std::shared_ptr<PropertyBase> &property : propertiesVec) {
    try {
      auto &newProperty =
          get(property->getName(), property->getInstanceName(),
              property->getClassName(), property->getProcessName());
      property->setValueString(newProperty->toString());
      newProperty = property;
    } catch (PropertyNotFoundException &exception) {
      properties.insert({property->getIdentifier(), property});
      updateRepository = true;
    }
  }
}

void PropertyService::changeStringValueOf(
    const std::shared_ptr<PropertyBase> &property, const std::string &value) {
  std::shared_ptr<PropertyBase> propertyBase =
      get(property->getName(), property->getInstanceName(),
          property->getClassName(), property->getProcessName());
  if (propertyBase->toString() == value) {
    return;
  }
  std::stringstream ss;
  ss << *property;
  LOG_INFO("{} change to {}", ss.str(), value);
  propertyBase->setValueString(value);
  propertyRepository->save(propertyBase);
}

void PropertyService::onInitialize() {
  AbstractService::onInitialize();
  if (updateRepository) {
    propertyRepository->save(allProperties());
  }
}
