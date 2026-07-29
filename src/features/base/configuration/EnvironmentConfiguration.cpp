#include "base_library/features/base/configuration/EnvironmentConfiguration.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>

#include "base_library/config.h"
#include "base_library/core/utils/StringUtils.h"

EnvironmentConfiguration::EnvironmentConfiguration() {
    // BootstarpConfigName
    const char *bootstrapConfigName = std::getenv("BOOTSTRAP_CONFIG_NAME");
    if (bootstrapConfigName != nullptr) {
        m_environmentConfigurations.insert(
                {BootstrapConfigName, bootstrapConfigName});
    } else {
        m_environmentConfigurations.insert(
                {BootstrapConfigName, BOOTSTRAP_CONFIG_NAME});
    }
    // ConfigDirectory
    const char *configDirectory = std::getenv("CONFIG_DIRECTORY");
    if (configDirectory != nullptr) {
        m_environmentConfigurations.insert({ConfigDirectory, configDirectory});
    } else {
        m_environmentConfigurations.insert({ConfigDirectory, CONFIG_DIRECTORY});
    }
    // HOME
    const char *home = std::getenv("HOME");
    if (home != nullptr) {
        m_environmentConfigurations.insert({Home, home});
    }
    // PATH
    const char *path = std::getenv("PATH");
    if (path != nullptr) {
        m_environmentConfigurations.insert({Path, path});
        std::vector<std::string> pathsStr = StringUtils::split(path, ':');
        for (const auto &iter: pathsStr) {
            m_paths.emplace_back(iter);
        }
    }
}

void EnvironmentConfiguration::overrides(Environment environment, std::string value) {
    if (environment == Environment::Path) {
        if (value.empty()) {
            // ignore empty path
            return;
        }
        auto found = m_environmentConfigurations.find(environment);
        if (found != m_environmentConfigurations.end()) {
            found->second = value;
        } else {
            m_environmentConfigurations.insert({environment, value});
        }
        std::vector<std::string> pathsStr = StringUtils::split(value, ':');
        m_paths.clear();
        for (const auto &iter: pathsStr) {
            m_paths.push_back((std::filesystem::path(iter)));
        }
        return;
    }
    auto found = m_environmentConfigurations.find(environment);
    if (found != m_environmentConfigurations.end()) {
        found->second = value;
        return;
    }
    m_environmentConfigurations.insert({environment, value});
}

std::vector<std::filesystem::path> EnvironmentConfiguration::pathsOf() {
    return m_paths;
}

std::optional<std::filesystem::path> EnvironmentConfiguration::pathOf(
        const std::string &fileName) {
    std::optional<std::filesystem::path> path = std::nullopt;
    for (const auto &iter: m_paths) {
        std::filesystem::path temp = iter;
        temp += std::filesystem::path::preferred_separator;
        temp += fileName;
        if (std::filesystem::exists(temp) &&
            std::filesystem::is_regular_file(temp)) {
            path = std::make_optional(temp);
            break;
        }
    }
    return path;
}

const std::string &EnvironmentConfiguration::of(
        EnvironmentConfiguration::Environment environment) {
    return m_environmentConfigurations.at(environment);
}