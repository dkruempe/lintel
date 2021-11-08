#include "base_library/features/property/services/PropertyService.h"

#include <algorithm>

#include "base_library/features/property/exceptions/PropertyNotFoundException.h"

std::map<std::string, std::shared_ptr<PropertyBase>> PropertyService::init(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories) {
  // sort repositories highest repository first
  std::vector<std::shared_ptr<PropertyRepository>> repositories(
      propertyRepositories);
  std::sort(repositories.begin(), repositories.end(),
            [](const std::shared_ptr<PropertyRepository> &r1,
               const std::shared_ptr<PropertyRepository> &r2) {
              return r1->getType() < r2->getType();
            });

  // create local map store
  std::map<std::string, std::shared_ptr<PropertyBase>> propertiesMap;

  // function to add properties to map
  std::function<void(std::vector<std::shared_ptr<PropertyBase>>)>
      addProperties =
          [&propertiesMap](const std::vector<std::shared_ptr<PropertyBase>>
                               &repoProperties) {
            for (const auto &property : repoProperties) {
              auto found = propertiesMap.find(property->getIdentifier());
              if (found != propertiesMap.end()) {
                found->second = property;
              } else {
                propertiesMap.insert({property->getIdentifier(), property});
              }
            }
          };

  // add properties to map
  for (const std::shared_ptr<PropertyRepository> &repository : repositories) {
    // highest property wins, bc. std::map insert only if not available
    addProperties(repository->awake());
  }
  return propertiesMap;
}

std::vector<std::shared_ptr<PropertyRepository>>
PropertyService::filterEnabledRepositories(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories) {
  std::vector<std::shared_ptr<PropertyRepository>> tmp;
  for (const auto &iter : propertyRepositories) {
    if (!iter->isEnabled()) {
      continue;
    }
    tmp.push_back(iter);
  }
  return tmp;
}

std::vector<std::shared_ptr<PropertyRepository>>
PropertyService::filterMutableRepositories(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepository) {
  std::vector<std::shared_ptr<PropertyRepository>> repositories(
      propertyRepository);
  std::sort(repositories.begin(), repositories.end(),
            [](const std::shared_ptr<PropertyRepository> &r1,
               const std::shared_ptr<PropertyRepository> &r2) {
              return r1->getType() > r2->getType();
            });

  std::vector<std::shared_ptr<PropertyRepository>> mutableRepositories;
  for (const std::shared_ptr<PropertyRepository> &repository : repositories) {
    if (!repository->isEnabled()) {
      continue;
    }
    if (repository->isMutable()) {
      mutableRepositories.push_back(repository);
    }
  }

  return mutableRepositories;
}

PropertyService::PropertyService(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories,
    std::vector<std::shared_ptr<AbstractServiceInterface>> abstractServices)
    : m_propertyRepositories(filterEnabledRepositories(propertyRepositories)),
      m_mutablePropertyRepositories(
          filterMutableRepositories(propertyRepositories)),
      m_abstractServices(std::move(abstractServices)),
      m_properties() {}
void PropertyService::onAwake() {
  m_properties = init(m_propertyRepositories);
  std::for_each(
      m_abstractServices.begin(), m_abstractServices.end(),
      [&](const std::shared_ptr<AbstractServiceInterface> &abstractService) {
        getOrCreate(abstractService->getProperties());
      });
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf() {
  std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
  std::transform(m_properties.begin(), m_properties.end(),
                 std::back_inserter(propertiesVector),
                 [](const auto &iter) { return iter.second; });
  return propertiesVector;
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
    const std::string &processName) {
  std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
  for (const auto &property : m_properties) {
    if (property.second->getProcessName() != processName) {
      continue;
    }
    propertiesVector.push_back(property.second);
  }
  return propertiesVector;
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
    const std::string &processName, const std::string &className) {
  std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
  for (const auto &property : m_properties) {
    if (property.second->getProcessName() != processName) {
      continue;
    }
    if (property.second->getClassName() != className) {
      continue;
    }
    propertiesVector.push_back(property.second);
  }
  return propertiesVector;
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
    const std::string &processName, const std::string &className,
    const std::string &instanceName) {
  std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
  for (const auto &property : m_properties) {
    if (property.second->getProcessName() != processName) {
      continue;
    }
    if (property.second->getClassName() != className) {
      continue;
    }
    if (property.second->getInstanceName() != instanceName) {
      continue;
    }
    propertiesVector.push_back(property.second);
  }
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
  auto found = m_properties.find(identifier);
  if (found == m_properties.end()) {
    throw PropertyNotFoundException(name, instanceName, className, processName);
  }
  return found->second;
}
void PropertyService::getOrCreate(
    const std::vector<std::shared_ptr<PropertyBase>> &propertiesVec) {
  std::vector<std::shared_ptr<PropertyBase>> tmp;
  for (const std::shared_ptr<PropertyBase> &property : propertiesVec) {
    try {
      auto &newProperty =
          get(property->getName(), property->getInstanceName(),
              property->getClassName(), property->getProcessName());
      // only overwrite value and Datastorage if value is different then default
      if (newProperty->toString() != property->toString()) {
        property->setValueString(newProperty->toString());
        property->setDataStorage(newProperty->getDataStorage());
        continue;
      }
      newProperty = property;
    } catch (PropertyNotFoundException &exception) {
      m_properties.insert({property->getIdentifier(), property});
      tmp.push_back(property);
    }
  }
  for (auto &iter : m_mutablePropertyRepositories) {
    if (!iter->isMutable() || !iter->isEnabled() || !iter->isShadow()) {
      continue;
    }
    iter->save(tmp);
  }
}

void PropertyService::changeStringValueOf(
    const std::shared_ptr<PropertyBase> &property, const std::string &value) {
  std::shared_ptr<PropertyBase> propertyBase =
      get(property->getName(), property->getInstanceName(),
          property->getClassName(), property->getProcessName());
  if (!propertyBase->isRuntimeChange()) {
    throw PropertyNoRuntimeChangeSupported(propertyBase);
  }
  if (propertyBase->toString() == value) {
    return;
  }
  std::stringstream ss;
  ss << *property;
  LOG_INFO("{} change to {}", ss.str(), value);
  propertyBase->setValueString(value);
  if (m_mutablePropertyRepositories.empty()) {
    LOG_ERROR("{} not mutable property repository");
    return;
  }
  // PropertyRepository with highest priority wins => DataStorage of the highest
  // priority is set
  propertyBase->setDataStorage(
      m_mutablePropertyRepositories[0]->getDataStorage());
  for (const auto &iter : m_mutablePropertyRepositories) {
    iter->save(propertyBase);
  }
}