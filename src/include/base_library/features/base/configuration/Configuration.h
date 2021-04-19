#ifndef CPP_BASE_LIBRARY_CONFIGURATION_H
#define CPP_BASE_LIBRARY_CONFIGURATION_H

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

#include "Component.h"
#include "Entry.h"
#include "base_library/core/utils/TypeName.h"

/**
 * General Configuration Parser
 * - parse xml file configuration
 * - modular architecture to provide add / delete configuration dynamically
 * - no save functionality
 * - goal is to provide a dynamic way to parse a defined configuration file with
 *   a give set of components
 * - get easy the configuration of the give components.
 */
class Configuration {
 private:
  std::map<std::string, std::shared_ptr<Component>> components;
  std::vector<std::shared_ptr<Entry>> configurationEntries;
  std::filesystem::path configurationFile;

  void loadConfiguration();

  static std::map<std::string, std::shared_ptr<Component>> initialize(
      const std::vector<std::shared_ptr<Component>> &tempComponents);

 public:
  explicit Configuration(
      const std::vector<std::shared_ptr<Component>> &components);

  // constructor for testing purposes
  void setEntries(std::vector<std::shared_ptr<Entry>> entries);

  template <typename COMPONENT>
  std::vector<std::shared_ptr<Entry>> configurationOf() {
    const std::string_view nameOfComponent = type_name<COMPONENT>();
    std::vector<std::shared_ptr<Entry>> tempEntries;
    for (auto &entry : configurationEntries) {
      if (entry->getConfigurationParserComponent() == nameOfComponent) {
        tempEntries.push_back(entry);
      }
    }
    return tempEntries;
  }
};

#endif  // CPP_BASE_LIBRARY_CONFIGURATION_H
