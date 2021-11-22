#ifndef CPP_BASE_LIBRARY_SHAREDMEMORYPROPERTYREPOSITORY_H
#define CPP_BASE_LIBRARY_SHAREDMEMORYPROPERTYREPOSITORY_H

#include <boost/interprocess/sync/named_upgradable_mutex.hpp>

#include "base_library/features/base/repositories/SharedMemoryRepository.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"
#include "base_library/features/property/repositories/PropertyRepository.h"

struct PropertyData {
  SharedMemoryService::ShmString m_processName;
  SharedMemoryService::ShmString m_className;
  SharedMemoryService::ShmString m_instanceName;
  SharedMemoryService::ShmString m_name;
  SharedMemoryService::ShmString m_value;
  SharedMemoryService::ShmString m_type;
};

class SharedMemoryPropertyRepository
    : public SharedMemoryMapRepository<SharedMemoryService::ShmString,
                                       PropertyData>,
      public PropertyRepository {
 private:
  static constexpr int32_t m_version = 0;
  std::shared_ptr<ProcessName> m_processName;
  DataStorage m_currentDataStorage =
      DataStorage(PropertyRepositoryType::SHM_REPOSITORY, "");
  boost::interprocess::named_upgradable_mutex m_upgradableMutex;

 public:
  SharedMemoryPropertyRepository(
      const std::shared_ptr<SharedMemoryService> &sharedMemoryService,
      const std::shared_ptr<SharedMemorySegmentManager>
          &sharedMemorySegmentManager,
      const std::shared_ptr<Configuration> &configuration,
      std::shared_ptr<ProcessName> processName);

  DataStorage getDataStorage() override;

  void save(
      const std::vector<std::shared_ptr<PropertyBase>> &properties) override;

  void save(std::shared_ptr<PropertyBase> property) override;

  std::vector<std::shared_ptr<PropertyBase>> awake() override;

  std::vector<std::shared_ptr<PropertyBase>> allOf(
      const std::string &processName, const std::string &className,
      const std::string &instanceName, const std::string &name) override;

  void onMigrate(int32_t currentActiveVersion) override;

  void deleteOf(
      const std::vector<std::shared_ptr<PropertyBase>> &properties) override;
};

#endif  // CPP_BASE_LIBRARY_SHAREDMEMORYPROPERTYREPOSITORY_H
