#ifndef PROPERTYREGISTRATION_H
#define PROPERTYREGISTRATION_H

#include <filesystem>
#include <source_location>

#include "lintel/core/models/Property.h"
#include "lintel/core/services/AbstractService.h"
#include "lintel/core/services/LoggerService.h"

namespace lintel {
/** CRTP base class that adds property registration capabilities to services. */
template<class T> class PropertyRegistration : public AbstractService<T>
{
protected:
  /** Register a property with the given configuration.
   * @tparam type        the property value type
   * @param name         the property name
   * @param defaultValue the default value
   * @param description  a human-readable description
   * @param runtime      whether the property can be changed at runtime
   * @param location     the source location of the registration (file and line) for data storage
   * @return a shared pointer to the registered property */
  template<class type>
  std::shared_ptr<Property<type>> registerProperty(std::string name,
    type defaultValue,
    std::string description,
    bool runtime,
    const std::source_location &location = std::source_location::current())
  {
    std::shared_ptr<Property<type>> property = std::make_shared<Property<type>>(name,
      this->getInstanceName(),
      std::string(this->getClassName()),
      this->getProcessName(),
      defaultValue,
      description,
      runtime);
    LOG_INFO("Property<{}> {} = {}", type_name<type>(), name, property->toString());
    const std::filesystem::path path(location.file_name());

    property->setDataStorage(DataStorage(
      PropertyRepositoryType::DEFAULT, path.filename().generic_string() + ":" + std::to_string(location.line())));
    this->m_properties.push_back(property);
    return property;
  }

public:
  using AbstractService<T>::AbstractService;
};

}// namespace lintel
#endif// PROPERTYREGISTRATION_H
