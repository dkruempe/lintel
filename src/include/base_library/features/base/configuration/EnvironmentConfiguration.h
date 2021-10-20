#pragma once

#include <map>
#include <string>

class EnvironmentConfiguration {
 public:
  enum Environment { ConfigDirectory, BootstrapConfigName };

  EnvironmentConfiguration();
  ~EnvironmentConfiguration() = default;

 private:
  std::map<Environment, std::string> m_environmentConfigurations;

  [[nodiscard]] const std::string& of(Environment environment);
};