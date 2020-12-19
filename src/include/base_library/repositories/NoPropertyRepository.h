#ifndef LOGGING_NOPROPERTYREPOSITORY_H
#define LOGGING_NOPROPERTYREPOSITORY_H

#include "PropertyRepository.h"

class NoPropertyRepository : public PropertyRepository {
public:
  NoPropertyRepository() : PropertyRepository() {}

  PropertyRepositoryType getType() override;

  bool isMutable() override;

  /**
   * save property to repository
   */
  void save(std::shared_ptr<PropertyBase> property) override;

  void
  save(const std::vector<std::shared_ptr<PropertyBase>> &properties) override;

  /**
   * load all properties from repository and return result
   */
  std::vector<std::shared_ptr<PropertyBase>> awake() override;
};

#endif // LOGGING_NOPROPERTYREPOSITORY_H
