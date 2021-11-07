#pragma once

#include <map>
#include <string>

class EnvironmentConfiguration {
 public:
  enum Environment { ConfigDirectory, BootstrapConfigName, Home };

  EnvironmentConfiguration();
  ~EnvironmentConfiguration() = default;

 private:
  std::map<Environment, std::string> m_environmentConfigurations;

 public:
  [[nodiscard]] const std::string& of(Environment environment);
};