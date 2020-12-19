#ifndef LOGGING_ABSTRACTSERVICE_H
#define LOGGING_ABSTRACTSERVICE_H

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "base_library/models/Property.h"
#include "base_library/utils/TypeName.h"
#include "base_library/services/LoggerService.h"

class PropertyService;

class AbstractServiceInterface {
 public:
  virtual void onInitialize() = 0;
  virtual void onShutdown() = 0;
 private:
  virtual std::vector<std::shared_ptr<PropertyBase>> getProperties() = 0;
  friend class PropertyService;
};
/**
 * Every Service must be derived from this service to make sure that basic
 * information gets provided
 *
 * These information are for example needed for initializing the Properties
 */
template <class T>
class AbstractService : public AbstractServiceInterface {
 private:
  const std::string processName;
  const std::string instanceName;

  std::vector<std::shared_ptr<PropertyBase>> getProperties() override {
    return properties;
  }

 protected:
  std::vector<std::shared_ptr<PropertyBase>> properties;

  template <class type>
  std::shared_ptr<Property<type>> registerProperty(std::string name,
                                                   type defaultValue,
                                                   std::string description,
                                                   bool runtime) {
    LOG_INFO("Property<{}> {} = {}", type_name<type>(), name, defaultValue);
    std::shared_ptr<Property<type>> property = std::make_shared<Property<type>>(
        name, getInstanceName(), std::string(getClassName()), getProcessName(),
        defaultValue, description, runtime);
    properties.push_back(property);
    return property;
  }

 public:
  AbstractService() = delete;
  AbstractService(std::string processName, std::string instanceName)
      : processName(std::move(processName)),
        instanceName(std::move(instanceName)) {}

  explicit AbstractService(std::string processName)
      : processName(std::move(processName)), instanceName("__DEFAULT") {}
  void onInitialize() override {}
  void onShutdown() override {}
  [[nodiscard]] const std::string &getProcessName() const {
    return processName;
  }
  [[nodiscard]] const std::string &getInstanceName() const {
    return instanceName;
  }
  [[nodiscard]] constexpr std::string_view getClassName() const {
    return type_name<T>();
  }
  friend std::ostream &operator<<(std::ostream &os,
                                  const AbstractService &service) {
    os << "processName: " << service.processName
       << " instanceName: " << service.instanceName
       << " className: " << service.getClassName();
    return os;
  }
};

#endif  // LOGGING_ABSTRACTSERVICE_H
