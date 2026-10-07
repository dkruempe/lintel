#ifndef LINTEL_CONFIGURATION_H
#define LINTEL_CONFIGURATION_H

#include <filesystem>
#include <map>
#include <memory>
#include <vector>

#include "Component.h"
#include "Entry.h"
#include "lintel/core/utils/TypeName.h"
#include "lintel/features/base/configuration/EnvironmentConfiguration.h"

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
    /** Map of component names to their component instances */
    std::map<std::string, std::shared_ptr<Component>> m_components;
    /** List of all parsed configuration entries */
    std::vector<std::shared_ptr<Entry>> m_configurationEntries;
    /** Environment configuration for resolving paths */
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;
    /** Path to the configuration file */
    std::filesystem::path m_configurationFile;

    /** Load and parse the configuration file */
    void loadConfiguration();

    /** Initialize the component map from a vector of components
     * @param tempComponents Vector of components to index
     * @return Map of component names to components */
    static std::map<std::string, std::shared_ptr<Component>> initialize(
            const std::vector<std::shared_ptr<Component>> &tempComponents);

public:
    /** Construct a Configuration parser
     * @param components The set of components used for parsing
     * @param EnvironmentConfiguration The environment configuration */
    explicit Configuration(
            const std::vector<std::shared_ptr<Component>> &components,
            std::shared_ptr<EnvironmentConfiguration> EnvironmentConfiguration);

    /** Set entries directly (for testing purposes)
     * @param entries The entries to set */
    void setEntries(std::vector<std::shared_ptr<Entry>> entries);

    /** Get all configuration entries for a specific component type
     * @tparam COMPONENT The component type to filter by
     * @return Vector of entries belonging to that component */
    template<typename COMPONENT>
    std::vector<std::shared_ptr<Entry>> configurationOf() {
        const std::string_view nameOfComponent = type_name<COMPONENT>();
        std::vector<std::shared_ptr<Entry>> tempEntries;
        for (auto &entry: m_configurationEntries) {
            if (entry->getConfigurationParserComponent() == nameOfComponent) {
                tempEntries.push_back(entry);
            }
        }
        return tempEntries;
    }
};

#endif  // LINTEL_CONFIGURATION_H
