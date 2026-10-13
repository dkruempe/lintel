#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

/** Provides environment-specific configuration values and file path resolution */
class EnvironmentConfiguration
{
public:
  /** Environment configuration keys */
  enum Environment {
    ConfigDirectory, /**< Configuration directory */
    BootstrapConfigName, /**< Bootstrap configuration file name */
    Home, /**< Home directory */
    Path /**< Search path */
  };

  /** Construct an environment configuration from the current process environment */
  EnvironmentConfiguration();

  /** Destructor */
  ~EnvironmentConfiguration() = default;

private:
  /** Map of environment keys to their string values */
  std::map<Environment, std::string> m_environmentConfigurations;
  /** List of configured search paths */
  std::vector<std::filesystem::path> m_paths;

public:
  /** Get the value for the given environment key
   * @param environment The environment key
   * @return The environment value */
  [[nodiscard]] const std::string &of(Environment environment);

  /** Get all configured search paths
   * @return Vector of filesystem paths */
  std::vector<std::filesystem::path> pathsOf();

  /** Find a file by name in the configured search paths
   * @param fileName The file name to search for
   * @return The full path if found, nullopt otherwise */
  std::optional<std::filesystem::path> pathOf(const std::string &fileName);

  /** Override an environment value
   * @param environment The environment key to override
   * @param value The new value */
  void overrides(Environment environment, std::string value);

  /**
   * Expand environment variables in the given string.
   *
   * Expands every occurrence of ${NAME} where NAME matches [A-Za-z_][A-Za-z0-9_]*.
   * - If NAME is set in the environment (std::getenv), its value is substituted.
   * - If NAME is not set, the literal ${NAME} text is left unchanged.
   * - Escaping: $${NAME} produces the literal ${NAME}.
   *
   * Note: This is a simple expansion and may expand ${...} inside XML comments
   * when the variable is set (this is acceptable for bootstrap configuration).
   *
   * @param value The string to expand
   * @return The expanded string
   */
  static std::string expand(const std::string &value);
};
