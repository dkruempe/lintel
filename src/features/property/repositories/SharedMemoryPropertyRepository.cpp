#include "base_library/features/property/repositories/SharedMemoryPropertyRepository.h"

#include <base_library/features/property/models/PropertyRepositoryType.h>

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <regex>

#include "base_library/features/property/factories/PropertyFactory.h"

PropertyDataDao::Shapes PropertyDataDao::m_shape{};

SharedMemoryPropertyRepository::SharedMemoryPropertyRepository(
    const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
    const std::shared_ptr<SharedMemorySegmentManager>
        &sharedMemorySegmentManager,
    const std::shared_ptr<Configuration> &configuration,
    std::shared_ptr<ProcessName> processName)
    : SharedMemoryMapRepository<SharedMemoryService::ShmString, PropertyDataDto,
                                SharedMemoryService::ShmString,
                                PropertyDataDao>(
          sharedMemoryService, sharedMemorySegmentManager->of("shm_property"),
          m_version, PropertyDataDto::getSize()),
      PropertyRepository(PropertyRepositoryType::SHM_REPOSITORY, configuration),
      m_processName(std::move(processName)),
      m_upgradableMutex(boost::interprocess::open_or_create,
                        "shm_property_mutex_upgradable") {}

DataStorage SharedMemoryPropertyRepository::getDataStorage() {
  return m_currentDataStorage;
}

void SharedMemoryPropertyRepository::save(
    std::shared_ptr<PropertyBase> property) {
  std::vector<std::shared_ptr<PropertyBase>> properties({property});
  save(properties);
}
void SharedMemoryPropertyRepository::save(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {
  // write lock
  boost::interprocess::scoped_lock<boost::interprocess::named_upgradable_mutex>
      lock(m_upgradableMutex);
  LOG_TRACE("save properties {}", getMap().size());
  std::shared_ptr<SharedMemorySegment> segment = getSharedMemorySegment();
  for (const auto &property : properties) {
    auto found = getMap().find(m_sharedMemoryService->constructString(
        segment, property->getIdentifier()));
    if (found != getMap().end()) {
      found->second.m_value =
          m_sharedMemoryService->constructString(segment, property->toString());
      continue;
    }
    SharedMemoryService::ShmString id = m_sharedMemoryService->constructString(
        segment, property->getIdentifier());
    PropertyDataDto propertyData{
        m_sharedMemoryService->constructString(segment,
                                               property->getProcessName()),

        m_sharedMemoryService->constructString(segment,
                                               property->getClassName()),

        m_sharedMemoryService->constructString(segment,
                                               property->getInstanceName()),

        m_sharedMemoryService->constructString(segment, property->getName()),
        m_sharedMemoryService->constructString(segment, property->toString()),
        m_sharedMemoryService->constructString(segment, property->getType())};
    getMap().insert({id, propertyData});
  }
  LOG_TRACE("finished save properties {}", getMap().size());
}
std::vector<std::shared_ptr<PropertyBase>>
SharedMemoryPropertyRepository::awake() {
  boost::interprocess::sharable_lock<
      boost::interprocess::named_upgradable_mutex>
      lock(m_upgradableMutex);
  LOG_TRACE("start awake {}", getMap().size());
  std::vector<std::shared_ptr<PropertyBase>> properties;
  for (const auto &[id, property] : getMap()) {
    std::shared_ptr<PropertyBase> tmp = PropertyFactory::Create(
        property.m_name.c_str(), property.m_instanceName.c_str(),
        property.m_className.c_str(), property.m_processName.c_str(),
        property.m_type.c_str(), property.m_value.c_str(), "", false);
    if (m_processName->getProcessName() != tmp->getProcessName()) {
      continue;
    }
    tmp->setDataStorage(m_currentDataStorage);
    properties.push_back(tmp);
    std::stringstream ss;
    ss << *tmp;
    LOG_TRACE("awake of {}", ss.str());
  }
  return properties;
}
std::vector<std::shared_ptr<PropertyBase>>
SharedMemoryPropertyRepository::allOf(const std::string &processName,
                                      const std::string &className,
                                      const std::string &instanceName,
                                      const std::string &name) {
  boost::interprocess::sharable_lock<
      boost::interprocess::named_upgradable_mutex>
      lock(m_upgradableMutex);
  LOG_TRACE("start awake {}", getMap().size());
  std::vector<std::shared_ptr<PropertyBase>> properties;
  std::regex processRegex(processName);
  std::regex classRegex(className);
  std::regex instanceRegex(instanceName);
  std::regex nameRegex(name);
  for (const auto &[id, property] : getMap()) {
    std::shared_ptr<PropertyBase> tmp = PropertyFactory::Create(
        property.m_name.c_str(), property.m_instanceName.c_str(),
        property.m_className.c_str(), property.m_processName.c_str(),
        property.m_type.c_str(), property.m_value.c_str(), "", false);
    if (!std::regex_match(tmp->getProcessName(), processRegex)) {
      continue;
    }
    if (!std::regex_match(tmp->getClassName(), classRegex)) {
      continue;
    }
    if (!std::regex_match(tmp->getInstanceName(), instanceRegex)) {
      continue;
    }
    if (!std::regex_match(tmp->getName(), nameRegex)) {
      continue;
    }
    tmp->setDataStorage(m_currentDataStorage);
    properties.push_back(tmp);
    std::stringstream ss;
    ss << *tmp;
    LOG_TRACE("awake of {}", ss.str());
  }
  return properties;
}
void SharedMemoryPropertyRepository::deleteOf(
    const std::vector<std::shared_ptr<PropertyBase>> &properties) {
  boost::interprocess::scoped_lock<boost::interprocess::named_upgradable_mutex>
      lock(m_upgradableMutex);
  LOG_INFO("start deleteOf with {}", properties.size());
  for (const auto &property : properties) {
    auto found = getMap().find(m_sharedMemoryService->constructString(
        getSharedMemorySegment(), property->getIdentifier()));
    if (found == getMap().end()) {
      continue;
    }
    getMap().erase(found);
    std::stringstream ss;
    ss << *property;
    LOG_TRACE("deleteOf({})", ss.str());
  }
}
void SharedMemoryPropertyRepository::onMigrate(int32_t currentActiveVersion) {}
bool PropertyDataDto::operator<(const PropertyDataDto &rhs) const {
  if (m_processName < rhs.m_processName) return true;
  if (rhs.m_processName < m_processName) return false;
  if (m_className < rhs.m_className) return true;
  if (rhs.m_className < m_className) return false;
  if (m_instanceName < rhs.m_instanceName) return true;
  if (rhs.m_instanceName < m_instanceName) return false;
  if (m_name < rhs.m_name) return true;
  if (rhs.m_name < m_name) return false;
  if (m_value < rhs.m_value) return true;
  if (rhs.m_value < m_value) return false;
  return m_type < rhs.m_type;
}
bool PropertyDataDto::operator>(const PropertyDataDto &rhs) const {
  return rhs < *this;
}
bool PropertyDataDto::operator<=(const PropertyDataDto &rhs) const {
  return !(rhs < *this);
}
bool PropertyDataDto::operator>=(const PropertyDataDto &rhs) const {
  return !(*this < rhs);
}
bool PropertyDataDto::operator==(const PropertyDataDto &rhs) const {
  return m_processName == rhs.m_processName && m_className == rhs.m_className &&
         m_instanceName == rhs.m_instanceName && m_name == rhs.m_name &&
         m_value == rhs.m_value && m_type == rhs.m_type;
}
bool PropertyDataDto::operator!=(const PropertyDataDto &rhs) const {
  return !(rhs == *this);
}
