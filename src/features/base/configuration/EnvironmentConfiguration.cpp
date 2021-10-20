#include "base_library/features/base/configuration/EnvironmentConfiguration.h"

#include <cstdlib>

#include "base_library/config.h"

EnvironmentConfiguration::EnvironmentConfiguration() {
  // BootstarpConfigName
  const char* bootstrapConfigName = std::getenv("BOOTSTRAP_CONFIG_NAME");
  if (bootstrapConfigName != nullptr) {
    m_environmentConfigurations.insert(
        {BootstrapConfigName, bootstrapConfigName});
  } else {
    m_environmentConfigurations.insert(
        {BootstrapConfigName, BOOTSTRAP_CONFIG_NAME});
  }
  // ConfigDirectory
  const char* configDirectory = std::getenv("CONFIG_DIRECTORY");
  if (configDirectory != nullptr) {
    m_environmentConfigurations.insert({ConfigDirectory, configDirectory});
  } else {
    m_environmentConfigurations.insert({ConfigDirectory, CONFIG_DIRECTORY});
  }
}

const std::string& EnvironmentConfiguration::of(
    EnvironmentConfiguration::Environment environment) {
  return m_environmentConfigurations.at(environment);
}