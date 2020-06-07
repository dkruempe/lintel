#include "services/PropertyService.h"
#include "exceptions/PropertyNotFoundException.h"
#include <algorithm>
std::vector<std::shared_ptr<PropertyBase>>
PropertyService::allProperties() {
  std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
  std::transform(properties.begin(), properties.end(),
                 std::back_inserter(propertiesVector),
                 [](const auto &iter) { return iter.second; });
  return propertiesVector;
}
std::string PropertyService::createIdentifier(const std::string &name,
                                              const std::string &instanceName,
                                              const std::string &processName) {
  return name + "_" + instanceName + "_" + processName;
}
std::shared_ptr<PropertyBase> PropertyService::get(const std::string &name,
                                   const std::string &instanceName,
                                   const std::string &processName) {
  const std::string &identifier =
      createIdentifier(name, instanceName, processName);
  auto found = properties.find(identifier);
  if (found == properties.end()) {
    throw PropertyNotFoundException(name, instanceName, processName);
  }
  return found->second;
}
