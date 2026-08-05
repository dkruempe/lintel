#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H

#include <base_library/core/configuration/EnvironmentConfiguration.h>

#include "Component.h"

namespace tinyxml2 {
class XMLElement;
}

/** Component for parsing database connection configurations from XML */
class DatabaseConnectionComponent : public Component {
private:
    /** The environment configuration for resolving environment variables */
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;
    /** XML element name constants for database connection parsing */
    static const struct Shapes {
        const char *const CONFIG_ROOT = "DatabaseConnections";
        const char *const DATABASE_ROOT = "DatabaseConnection";
        const char *const CONNECTION_NAME = "name";
        const char *const CONNECTION_TYPE = "type";
        const char *const CONNECTION_USER_NAME = "user_name";
        const char *const CONNECTION_PASSWORD = "password";
        const char *const CONNECTION_CONNECTION = "connection";
        const char *const CONNECTION_DATBASE_NAME = "database_name";
        const char *const CONNECTION_PORT = "port";
        const char *const CONNECTION_DEFAULT = "default";
    } shape;

    /** Parse a single database connection entry from XML
     * @param propertyElement The XML element to parse
     * @param lineOffset Line offset for error reporting
     * @param configRoot The configuration root name
     * @param envConfig Environment configuration for resolving variables
     * @param foundConnectionWithDefault Output flag indicating a default connection was found
     * @return Parsed entry or nullptr */
    static std::shared_ptr<Entry> parseDatabaseEntry(
            tinyxml2::XMLElement *propertyElement, int32_t lineOffset,
            const std::string &configRoot,
            const std::shared_ptr<EnvironmentConfiguration> &envConfig,
            bool &foundConnectionWithDefault);

public:
    /** Construct a DatabaseConnectionComponent
     * @param environmentConfiguration The environment configuration */
    DatabaseConnectionComponent(
            std::shared_ptr<EnvironmentConfiguration> environmentConfiguration);

    /** Parse database connection configuration
     * @param content XML content to parse
     * @param fileName Source file name for error reporting
     * @param lineOffset Line offset for error reporting
     * @return Vector of parsed database connection entries */
    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H
