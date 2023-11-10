#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class EnvironmentConfiguration {
public:
    enum Environment {
        ConfigDirectory, BootstrapConfigName, Home, Path
    };

    EnvironmentConfiguration();

    ~EnvironmentConfiguration() = default;

private:
    std::map<Environment, std::string> m_environmentConfigurations;
    std::vector<std::filesystem::path> m_paths;

public:
    [[nodiscard]] const std::string &of(Environment environment);

    // PATH belonging functions
    std::vector<std::filesystem::path> pathsOf();

    std::optional<std::filesystem::path> pathOf(const std::string &fileName);

    void overrides(Environment environment, std::string value);
};