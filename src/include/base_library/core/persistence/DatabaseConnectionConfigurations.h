#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H

#include <map>
#include <vector>

#include "base_library/core/configuration/Configuration.h"
#include "base_library/core/configuration/DatabaseConnectionEntry.h"

/**
 * Manages database connection configurations loaded from a Configuration object.
 */
class DatabaseConnectionConfigurations {
private:
    std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>> m_connections;
    std::shared_ptr<DatabaseConnectionEntry> m_defaultConnection;

    /**
     * Builds a name-to-entry map from a list of configuration entries.
     * @param entries list of configuration entries
     * @return map of name to DatabaseConnectionEntry
     */
    static std::map<std::string, std::shared_ptr<DatabaseConnectionEntry>> build(
            std::vector<std::shared_ptr<Entry>> entries);

    /**
     * Finds the default connection entry from the list.
     * @param entries list of configuration entries
     * @return the default entry, or nullptr if none is marked default
     */
    static std::shared_ptr<DatabaseConnectionEntry> findDefault(
            const std::vector<std::shared_ptr<Entry>> &entries);

public:
    /**
     * Constructs configurations from a Configuration object.
     * @param configuration the configuration containing database connection entries
     */
    explicit DatabaseConnectionConfigurations(
            const std::shared_ptr<Configuration> &configuration);

    /** @return the default database connection entry */
    [[nodiscard]] std::shared_ptr<DatabaseConnectionEntry> ofDefault();

    /**
     * Looks up a connection entry by name.
     * @param connectionName the connection name
     * @return the matching entry, or nullptr if not found
     */
    [[nodiscard]] std::shared_ptr<DatabaseConnectionEntry> of(
            const std::string &connectionName);

    /** @return list of all connection entries */
    [[nodiscard]] std::vector<std::shared_ptr<DatabaseConnectionEntry>> allOf();
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONCONFIGURATIONS_H
