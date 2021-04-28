#ifndef CPP_BASE_LIBRARY_DATABASEPROPERTYREPOSITORY_H
#define CPP_BASE_LIBRARY_DATABASEPROPERTYREPOSITORY_H

#include <memory>

#include "base_library/core/persistence/ConnectionConfigurations.h"
#include "base_library/features/base/configuration/ConnectionEntry.h"
#include "base_library/features/property/repositories/PropertyRepository.h"

class DatabasePropertyRepository : public PropertyRepository {
 private:
  std::shared_ptr<ConnectionConfigurations> connectionConfigurations;
  std::shared_ptr<ConnectionEntry> connectionEntry;

 public:
  explicit DatabasePropertyRepository(
      std::shared_ptr<ConnectionConfigurations> connectionConfigurations);

  PropertyRepositoryType getType() override;

  DataStorage getDataStorage() override;

  /**
   * returns if save operations are supported or not
   */
  bool isMutable() override;

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
