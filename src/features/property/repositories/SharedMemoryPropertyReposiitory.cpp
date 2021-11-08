#include <base_library/features/property/models/PropertyRepositoryType.h>

#include <regex>

#include "base_library/features/property/factories/PropertyFactory.h"
#include "base_library/features/property/repositories/SharedMemoryPropertyRepository.h"

SharedMemoryPropertyRepository::SharedMemoryPropertyRepository(
    const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
    const std::shared_ptr<SharedMemorySegmentManager>
        &sharedMemorySegmentManager,
    const std::shared_ptr<Configuration> &configuration,
    std::shared_ptr<ProcessName> processName)
    : SharedMemoryMapRepository<SharedMemoryService::ShmString, PropertyData>(
          sharedMemoryService, sharedMemorySegmentManager->of("shm_property"),
          m_version),
      PropertyRepository(PropertyRepositoryType::SHM_REPOSITORY, configuration),
      m_processName(std::move(processName)) {}

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
  LOG_TRACE("save properties {}", getMap().size());
  for (auto &property : properties) {
    auto found = getMap().find(m_sharedMemoryService->constructString(
        getSharedMemorySegment()->getName(), property->getIdentifier()));
    if (found != getMap().end()) {
      found->second.m_value = m_sharedMemoryService->constructString(
          getSharedMemorySegment()->getName(), property->toString());
      continue;
    } else {
      SharedMemoryService::ShmString id =
          m_sharedMemoryService->constructString(
              getSharedMemorySegment()->getName(), property->getIdentifier());
      PropertyData propertyData{
          m_sharedMemoryService->constructString(
              getSharedMemorySegment()->getName(), property->getProcessName()),
          m_sharedMemoryService->constructString(
              getSharedMemorySegment()->getName(), property->getClassName()),
          m_sharedMemoryService->constructString(
              getSharedMemorySegment()->getName(), property->getInstanceName()),
          m_sharedMemoryService->constructString(
              getSharedMemorySegment()->getName(), property->getName()),
          m_sharedMemoryService->constructString(
              getSharedMemorySegment()->getName(), property->toString()),
          m_sharedMemoryService->constructString(
              getSharedMemorySegment()->getName(), property->getType())};
      getMap().insert({id, propertyData});
    }
  }
  LOG_TRACE("finished save properties {}", getMap().size());
}
std::vector<std::shared_ptr<PropertyBase>>
SharedMemoryPropertyRepository::awake() {
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
void SharedMemoryPropertyRepository::onMigrate(int32_t currentActiveVersion) {}
