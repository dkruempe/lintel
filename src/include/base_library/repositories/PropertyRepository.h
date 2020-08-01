#ifndef LOGGING_PROPERTYREPOSITORY_H
#define LOGGING_PROPERTYREPOSITORY_H

#include <memory>

class PropertyBase;

class PropertyRepository {
public:

  PropertyRepository() = default;

  /**
   * save properties to repository
   */
  virtual void
  save(const std::vector<std::shared_ptr<PropertyBase>> &properties) = 0;

  /**
   * save property to repository
   */
  virtual void save(std::shared_ptr<PropertyBase> property) = 0;

  /**
   * load all properties from repository and return result
   */
  virtual std::vector<std::shared_ptr<PropertyBase>> awake() = 0;
};

#endif // LOGGING_PROPERTYREPOSITORY_H
