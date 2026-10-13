#include "lintel/features/base/configuration/EnvironmentConfiguration.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>

#include "lintel/config.h"
#include "lintel/core/utils/StringUtils.h"

namespace lintel {
EnvironmentConfiguration::EnvironmentConfiguration()
{
  // BootstarpConfigName
  const char *bootstrapConfigName = std::getenv("BOOTSTRAP_CONFIG_NAME");
  if (bootstrapConfigName != nullptr) {
    m_environmentConfigurations.insert({ BootstrapConfigName, bootstrapConfigName });
  } else {
    m_environmentConfigurations.insert({ BootstrapConfigName, BOOTSTRAP_CONFIG_NAME });
  }
  // ConfigDirectory
  const char *configDirectory = std::getenv("CONFIG_DIRECTORY");
  if (configDirectory != nullptr) {
    m_environmentConfigurations.insert({ ConfigDirectory, configDirectory });
  } else {
    m_environmentConfigurations.insert({ ConfigDirectory, CONFIG_DIRECTORY });
  }
  // HOME
  const char *home = std::getenv("HOME");
  if (home != nullptr) { m_environmentConfigurations.insert({ Home, home }); }
  // PATH
  const char *path = std::getenv("PATH");
  if (path != nullptr) {
    m_environmentConfigurations.insert({ Path, path });
    std::vector<std::string> pathsStr = StringUtils::split(path, ':');
    for (const auto &iter : pathsStr) { m_paths.emplace_back(iter); }
  }
}

void EnvironmentConfiguration::overrides(Environment environment, std::string value)
{
  if (environment == Environment::Path) {
    if (value.empty()) {
      // ignore empty path
      return;
    }
    auto found = m_environmentConfigurations.find(environment);
    if (found != m_environmentConfigurations.end()) {
      found->second = value;
    } else {
      m_environmentConfigurations.insert({ environment, value });
    }
    std::vector<std::string> pathsStr = StringUtils::split(value, ':');
    m_paths.clear();
    for (const auto &iter : pathsStr) { m_paths.push_back((std::filesystem::path(iter))); }
    return;
  }
  auto found = m_environmentConfigurations.find(environment);
  if (found != m_environmentConfigurations.end()) {
    found->second = value;
    return;
  }
  m_environmentConfigurations.insert({ environment, value });
}

std::vector<std::filesystem::path> EnvironmentConfiguration::pathsOf() { return m_paths; }

std::optional<std::filesystem::path> EnvironmentConfiguration::pathOf(const std::string &fileName)
{
  std::optional<std::filesystem::path> path = std::nullopt;
  for (const auto &iter : m_paths) {
    std::filesystem::path temp = iter;
    temp += std::filesystem::path::preferred_separator;
    temp += fileName;
    if (std::filesystem::exists(temp) && std::filesystem::is_regular_file(temp)) {
      path = std::make_optional(temp);
      break;
    }
  }
  return path;
}

const std::string &EnvironmentConfiguration::of(EnvironmentConfiguration::Environment environment)
{
  return m_environmentConfigurations.at(environment);
}

std::string EnvironmentConfiguration::expand(const std::string &value)
{
  std::string result;
  result.reserve(value.size());
  const std::size_t len = value.size();
  std::size_t i = 0;
  while (i < len) {
    // "$${NAME}" escapes to the literal "${NAME}"
    if (value[i] == '$' && i + 1 < len && value[i + 1] == '$') {
      result += '$';
      i += 2;
      continue;
    }
    // "${NAME}" with NAME = [A-Za-z_][A-Za-z0-9_]*
    if (value[i] == '$' && i + 1 < len && value[i + 1] == '{') {
      std::size_t nameStart = i + 2;
      std::size_t end = nameStart;
      if (end < len && (std::isalpha(static_cast<unsigned char>(value[end])) || value[end] == '_')) {
        ++end;
        while (end < len && (std::isalnum(static_cast<unsigned char>(value[end])) || value[end] == '_')) { ++end; }
        if (end < len && value[end] == '}') {
          const std::string name = value.substr(nameStart, end - nameStart);
          const char *environmentValue = std::getenv(name.c_str());
          if (environmentValue != nullptr) {
            result += environmentValue;
          } else {
            // unset variable stays literal
            result.append(value, i, end - i + 1);
          }
          i = end + 1;
          continue;
        }
      }
    }
    result += value[i];
    ++i;
  }
  return result;
}
}// namespace lintel
