#ifndef LOGGING_PROPERTYREPOSITORY_H
#define LOGGING_PROPERTYREPOSITORY_H

#include <memory>

class PropertyBase;

class PropertyRepository {
public:
  /**
   * Notification class support the possiblity to get notified in case of
   * change of the property in the repository.
   */
  struct Notification {
  public:
    virtual void notifyChangeOf(std::shared_ptr<PropertyBase> property) = 0;
  };

  PropertyRepository() = delete;

  explicit PropertyRepository(Notification &notification)
      : notification(notification) {}

  /**
   * save property to repository
   */
  virtual void save(std::shared_ptr<PropertyBase> property) = 0;

  /**
   * load all properties from repository and return result
   */
  virtual std::vector<std::shared_ptr<PropertyBase>> awake() = 0;

protected:
  Notification &notification;
};

#endif // LOGGING_PROPERTYREPOSITORY_H
