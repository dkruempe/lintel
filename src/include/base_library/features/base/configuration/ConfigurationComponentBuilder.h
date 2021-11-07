#pragma once

#include <memory>
#include <vector>

#include "base_library/features/base/configuration/Component.h"
#include "base_library/features/base/configuration/EnvironmentConfiguration.h"


class ConfigurationComponentBuilder {
 private:
  std::vector<std::shared_ptr<Component>> m_components;

 public:
  ConfigurationComponentBuilder(const std::shared_ptr<EnvironmentConfiguration>
                                    &environmentConfiguration);
  ~ConfigurationComponentBuilder() = default;
  void add(std::shared_ptr<Component> &&component);
  void add(const std::shared_ptr<Component> &component);
  [[nodiscard]] const std::vector<std::shared_ptr<Component>> &build();
};