#ifndef LOGGING_ABSTRACTSERVICE_H
#define LOGGING_ABSTRACTSERVICE_H

#include "models/Property.h"
#include "utils/TypeName.h"
#include <ostream>
#include <string>
#include <utility>
#include <vector>
/**
 * Every Service must be derived from this service to make sure that basic
 * information gets provided
 *
 * These information are for example needed for initializing the Properties
 */
template <class T> class AbstractService {
private:
  const std::string processName;
  const std::string instanceName;

protected:
  std::vector<std::shared_ptr<PropertyBase>> properties;

  template <class type>
  std::shared_ptr<Property<type>> registerProperty(std::string name,
                                                   type defaultValue) {
    std::shared_ptr<Property<type>> property = std::make_shared<Property<type>>(
        name, getInstanceName(), std::string(getClassName()), getProcessName(),
        defaultValue);
    properties.push_back(property);
    return property;
  }

public:
  AbstractService() = delete;
  AbstractService(std::string processName, std::string instanceName)
      : processName(std::move(processName)),
        instanceName(std::move(instanceName)) {}
  virtual void onInitialize() {}
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

#endif // LOGGING_ABSTRACTSERVICE_H
