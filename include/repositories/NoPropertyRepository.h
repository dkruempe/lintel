#ifndef LOGGING_NOPROPERTYREPOSITORY_H
#define LOGGING_NOPROPERTYREPOSITORY_H

#include "repositories/PropertyRepository.h"

class NoPropertyRepository : public PropertyRepository {
public:

  struct EmptyNotification : public Notification {
    void notifyChangeOf(std::shared_ptr<PropertyBase> property) override {}
  };

  EmptyNotification emptyNotification;

  NoPropertyRepository() : PropertyRepository(emptyNotification) {}

  /**
   * save property to repository
   */
  void save(std::shared_ptr<PropertyBase> property) override;

  /**
   * load all properties from repository and return result
   */
  std::vector<std::shared_ptr<PropertyBase>> awake() override;
};

#endif // LOGGING_NOPROPERTYREPOSITORY_H
