#include "base_library/features/property/services/PropertyService.h"

#include <algorithm>
#include <regex>
#include <sstream>

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
      LOG_TRACE("mutableRepositories({})", repository->getType().toString());
      mutableRepositories.push_back(repository);
    }
  }

  return mutableRepositories;
}

std::vector<std::shared_ptr<PropertyRepository>>
PropertyService::filterShadowRepositories(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepository) {
  std::vector<std::shared_ptr<PropertyRepository>> repositories(
      propertyRepository);
  std::sort(repositories.begin(), repositories.end(),
            [](const std::shared_ptr<PropertyRepository> &r1,
               const std::shared_ptr<PropertyRepository> &r2) {
              return r1->getType() > r2->getType();
            });

  std::vector<std::shared_ptr<PropertyRepository>> shadowRepositories;
  for (const std::shared_ptr<PropertyRepository> &repository : repositories) {
    if (!repository->isEnabled()) {
      continue;
    }
    if (repository->isMutable() && repository->isShadow()) {
      LOG_TRACE("shadowRepositories({})", repository->getType().toString());
      shadowRepositories.push_back(repository);
    }
  }

  return shadowRepositories;
}

PropertyService::PropertyService(
    const std::vector<std::shared_ptr<PropertyRepository>>
        &propertyRepositories,
    std::vector<std::shared_ptr<AbstractServiceInterface>> abstractServices)
    : m_propertyRepositories(filterEnabledRepositories(propertyRepositories)),
      m_mutablePropertyRepositories(
          filterMutableRepositories(propertyRepositories)),
      m_shadowPropertyRepositories(
          filterShadowRepositories(propertyRepositories)),
      m_abstractServices(std::move(abstractServices)),
      m_properties() {}
void PropertyService::onAwake() {
  m_properties = init(m_propertyRepositories);
  std::vector<std::shared_ptr<PropertyBase>> properties;
  for (const auto &iter : m_abstractServices) {
    for (const auto &property : iter->getProperties()) {
      properties.push_back(property);
    }
  }
  getOrCreate(properties);
  // std::for_each(
  //     m_abstractServices.begin(), m_abstractServices.end(),
  //     [&](const std::shared_ptr<AbstractServiceInterface> &abstractService) {
  //       getOrCreate(abstractService->getProperties());
  //     });
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf() {
  if (m_shadowPropertyRepositories.empty()) {
    std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
    std::transform(m_properties.begin(), m_properties.end(),
                   std::back_inserter(propertiesVector),
                   [](const auto &iter) { return iter.second; });
    return propertiesVector;
  }
  return m_shadowPropertyRepositories[0]->allOf();
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
    const std::string &processName) {
  if (m_shadowPropertyRepositories.empty()) {
    std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
    std::transform(m_properties.begin(), m_properties.end(),
                   std::back_inserter(propertiesVector),
                   [](const auto &iter) { return iter.second; });
    return propertiesVector;
  }
  return m_shadowPropertyRepositories[0]->allOf(processName);
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
    const std::string &processName, const std::string &className) {
  if (m_shadowPropertyRepositories.empty()) {
    std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
    std::regex processRegex(processName);
    std::regex classRegex(className);
    for (const auto &property : m_properties) {
      if (!std::regex_match(property.second->getProcessName(), processRegex)) {
        continue;
      }
      if (!std::regex_match(property.second->getClassName(), classRegex)) {
        continue;
      }
      propertiesVector.push_back(property.second);
    }
    return propertiesVector;
  }
  return m_shadowPropertyRepositories[0]->allOf(processName, className);
}
std::vector<std::shared_ptr<PropertyBase>> PropertyService::allOf(
    const std::string &processName, const std::string &className,
    const std::string &instanceName) {
  LOG_TRACE("shadowPropertyRepositories({})",
            m_shadowPropertyRepositories.size());
  if (m_shadowPropertyRepositories.empty()) {
    std::vector<std::shared_ptr<PropertyBase>> propertiesVector;
    std::regex processRegex(processName);
    std::regex classRegex(className);
    std::regex instanceRegex(instanceName);
    LOG_TRACE("({},{},{}) properties({})", processName, className, instanceName,
              m_properties.size());
    for (const auto &property : m_properties) {
      if (!std::regex_match(property.second->getProcessName(), processRegex)) {
        continue;
      }
      if (!std::regex_match(property.second->getClassName(), classRegex)) {
        continue;
      }
      if (!std::regex_match(property.second->getInstanceName(),
                            instanceRegex)) {
        continue;
      }
      propertiesVector.push_back(property.second);
    }
    return propertiesVector;
  }
  return m_shadowPropertyRepositories[0]->allOf(processName, className,
                                                instanceName);
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
  for (const std::shared_ptr<PropertyBase> &property : propertiesVec) {
    try {
      auto &newProperty =
          get(property->getName(), property->getInstanceName(),
              property->getClassName(), property->getProcessName());
      // only overwrite value and Datastorage if value is different then default
      if (newProperty->toString() != property->toString()) {
        property->setValueString(newProperty->toString());
        property->setDataStorage(newProperty->getDataStorage());
      }
      newProperty = property;
    } catch (PropertyNotFoundException &exception) {
      m_properties.insert({property->getIdentifier(), property});
    }
  }
  for (auto &iter : m_mutablePropertyRepositories) {
    LOG_TRACE(
        "mutable properties repositories {} with (isEnabled, isMutable, "
        "isShadow)/({},{},{})",
        iter->getType().toString(), iter->isEnabled(), iter->isMutable(),
        iter->isShadow());
    if (!iter->isShadow()) {
      LOG_INFO("start deleting of shadow properties {}",
               iter->getType().toString());
      std::vector<std::shared_ptr<PropertyBase>> propertiesCopy = propertiesVec;
      propertiesCopy.erase(
          std::remove_if(
              propertiesCopy.begin(), propertiesCopy.end(),
              [&](const std::shared_ptr<PropertyBase> &property) -> bool {
                return property->getDataStorage().getType() == iter->getType();
              }),
          propertiesCopy.end());
      iter->deleteOf(propertiesCopy);
      continue;
    }
    if (!iter->isMutable() || !iter->isEnabled()) {
      continue;
    }
    // TODO optimize software wise ?!
    iter->save(propertiesVec);
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