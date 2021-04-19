#ifndef LOGGING_PROPERTYREPOSITORY_H
#define LOGGING_PROPERTYREPOSITORY_H

#include <memory>
#include <vector>

#include "base_library/features/property/models/DataStorage.h"
#include "base_library/features/property/models/PropertyRepositoryType.h"

class PropertyBase;

class PropertyRepository {
 public:
  PropertyRepository() = default;

  /**
   * returns priority of repository
   */
  virtual PropertyRepositoryType getType() = 0;

  virtual DataStorage getDataStorage() = 0;

  /**
   * returns if save operations are supported or not
   */
  virtual bool isMutable() = 0;

  /**
   * save properties to repository
   */
  virtual void save(
      const std::vector<std::shared_ptr<PropertyBase>> &properties) = 0;

  /**
   * save property to repository
   */
  virtual void save(std::shared_ptr<PropertyBase> property) = 0;

  /**
   * load all properties from repository and return result
   */
  virtual std::vector<std::shared_ptr<PropertyBase>> awake() = 0;
};

#endif  // LOGGING_PROPERTYREPOSITORY_H
