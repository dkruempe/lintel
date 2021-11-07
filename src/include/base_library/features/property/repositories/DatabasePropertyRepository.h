#ifndef CPP_BASE_LIBRARY_DATABASEPROPERTYREPOSITORY_H
#define CPP_BASE_LIBRARY_DATABASEPROPERTYREPOSITORY_H

#include <memory>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"
#include "base_library/features/property/repositories/PropertyRepository.h"

class DatabasePropertyRepository : public PropertyRepository {
 private:
  std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
  std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
  DataStorage m_currentDataStorage =
      DataStorage(PropertyRepositoryType::DATABASE_REPOSITORY, "");

 public:
  explicit DatabasePropertyRepository(
      std::shared_ptr<DatabaseConnectionConfigurations>
          connectionConfigurations,
      const std::shared_ptr<Configuration> &configuration);

  DataStorage getDataStorage() override;

  /**
   * save properties to repository
   */
  void save(
      const std::vector<std::shared_ptr<PropertyBase>> &properties) override;

  /**
   * save property to repository
   */
  void save(std::shared_ptr<PropertyBase> property) override;

  /**
   * load all properties from repository and return result
   */
  std::vector<std::shared_ptr<PropertyBase>> awake() override;
};

#endif  // CPP_BASE_LIBRARY_DATABASEPROPERTYREPOSITORY_H
