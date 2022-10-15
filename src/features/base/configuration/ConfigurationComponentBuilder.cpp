#include "base_library/features/base/configuration/ConfigurationComponentBuilder.h"

#include <memory>

#include "base_library/features/base/configuration/DatabaseConnectionComponent.h"
#include "base_library/features/base/configuration/LoggerComponent.h"
#include "base_library/features/base/configuration/ProcessComponent.h"
#include "base_library/features/base/configuration/SharedMemorySegmentComponent.h"
#include "base_library/features/http/configuration/HttpComponent.h"
#include "base_library/features/property/configuration/PropertyComponent.h"
#include "base_library/features/property/configuration/PropertyRepositoryComponent.h"

ConfigurationComponentBuilder::ConfigurationComponentBuilder(
    const std::shared_ptr<EnvironmentConfiguration> &environmentConfiguration) {
  m_components.push_back(
      std::make_shared<DatabaseConnectionComponent>(environmentConfiguration));
  m_components.push_back(std::make_shared<HttpComponent>());
  m_components.push_back(std::make_shared<PropertyComponent>());
  m_components.push_back(
      std::make_shared<LoggerComponent>(environmentConfiguration));
  m_components.push_back(
      std::make_shared<SharedMemorySegmentComponent>(environmentConfiguration));
  m_components.push_back(std::make_shared<PropertyRepositoryComponent>());
  m_components.push_back(std::make_shared<ProcessComponent>());
}
void ConfigurationComponentBuilder::add(
    std::shared_ptr<Component> &&component) {
  m_components.push_back(std::move(component));
}
void ConfigurationComponentBuilder::add(
    const std::shared_ptr<Component> &component) {
  m_components.push_back(component);
}
const std::vector<std::shared_ptr<Component>>
    &ConfigurationComponentBuilder::build() {
  return m_components;
}