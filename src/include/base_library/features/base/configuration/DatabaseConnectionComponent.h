#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H

#include <base_library/features/base/configuration/EnvironmentConfiguration.h>

#include "Component.h"

namespace tinyxml2 {
class XMLElement;
}

class DatabaseConnectionComponent : public Component {
private:
    std::shared_ptr<EnvironmentConfiguration> m_environmentConfiguration;
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

    static std::shared_ptr<Entry> parseDatabaseEntry(
            tinyxml2::XMLElement *propertyElement, int32_t lineOffset,
            const std::string &configRoot,
            const std::shared_ptr<EnvironmentConfiguration> &envConfig,
            bool &foundConnectionWithDefault);

public:
    DatabaseConnectionComponent(
            std::shared_ptr<EnvironmentConfiguration> environmentConfiguration);

    std::vector<std::shared_ptr<Entry>> parse(const std::string &content,
                                              const std::string &fileName,
                                              int32_t lineOffset) override;
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONCOMPONENT_H
