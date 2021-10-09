#include "base_library/features/base/configuration/ConfigurationComponentBuilder.h"

#include <memory>

#include "base_library/features/base/configuration/DatabaseConnectionComponent.h"
#include "base_library/features/http/configuration/HttpComponent.h"
#include "base_library/features/property/configuration/PropertyComponent.h"

ConfigurationComponentBuilder::ConfigurationComponentBuilder() {
  m_components.push_back(std::make_shared<DatabaseConnectionComponent>());
  m_components.push_back(std::make_shared<HttpComponent>());
  m_components.push_back(std::make_shared<PropertyComponent>());
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