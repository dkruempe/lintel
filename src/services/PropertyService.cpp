#include "base_library/services/PropertyService.h"

#include <algorithm>

#include "base_library/exceptions/PropertyNotFoundException.h"

std::map<std::string, std::shared_ptr<PropertyBase>> PropertyService::init(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories) {
  // sort repositories highest repository first
  std::vector<std::shared_ptr<PropertyRepository>> repositories(
      propertyRepositories);
  std::sort(repositories.begin(), repositories.end(),
            [](const std::shared_ptr<PropertyRepository> &r1,
               const std::shared_ptr<PropertyRepository> &r2) {
              return r1->getPriority() > r2->getPriority();
            });

  // create local map store
  std::map<std::string, std::shared_ptr<PropertyBase>> propertiesMap;

  // function to add properties to map
  std::function<void(std::vector<std::shared_ptr<PropertyBase>>)>
      addProperties =
          [&propertiesMap](const std::vector<std::shared_ptr<PropertyBase>>
                               &repoProperties) {
            for (const auto &property : repoProperties) {
              propertiesMap.insert({property->getIdentifier(), property});
            }
          };

  // add properties to map
  for (const std::shared_ptr<PropertyRepository> &repository : repositories) {
    // highest property wins, bc. std::map insert only if not available
    addProperties(repository->awake());
  }
  return propertiesMap;
}

std::shared_ptr<PropertyRepository> PropertyService::searchRuntimeRepository(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepository) {
  std::vector<std::shared_ptr<PropertyRepository>> repositories(
      propertyRepository);
  std::sort(repositories.begin(), repositories.end(),
            [](const std::shared_ptr<PropertyRepository> &r1,
               const std::shared_ptr<PropertyRepository> &r2) {
              return r1->getPriority() > r2->getPriority();
            });

  for (const std::shared_ptr<PropertyRepository> &repository : repositories) {
    if (repository->isMutable()) {
      return repository;
    }
  }

  return nullptr;
}

PropertyService::PropertyService(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories,
    const std::shared_ptr<ProcessName> &processName)
    : AbstractService(processName->getProcessName(), "PropertyService"),
      propertyRepository(searchRuntimeRepository(propertyRepositories)),
      properties(init(propertyRepositories)) {}
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
  if (propertyRepository != nullptr) {
    propertyRepository->save(propertyBase);
  }
}

void PropertyService::onInitialize() {
  AbstractService::onInitialize();
  if (updateRepository && propertyRepository != nullptr) {
    propertyRepository->save(allProperties());
  }
}
