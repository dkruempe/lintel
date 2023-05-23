#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONENTRY_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONENTRY_H

#include <ostream>

#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/features/base/configuration/Entry.h"

class DatabaseConnectionEntry : public Entry {
private:
    const std::string m_connection;
    const std::string m_userName;
    const std::string m_password;
    db::ConnectionType m_type;
    const std::string m_name;
    const int32_t m_port;
    const std::string m_databaseName;
    bool m_isDefault;

public:
    DatabaseConnectionEntry(std::string_view component, std::string connection,
                            std::string userName, std::string password,
                            db::ConnectionType type, std::string name,
                            int32_t port, std::string databaseName,
                            bool isDefault);

    [[nodiscard]] const std::string &getConnection() const;

    [[nodiscard]] const std::string &getUserName() const;

    [[nodiscard]] const std::string &getPassword() const;

    [[nodiscard]] const db::ConnectionType &getType() const;

    [[nodiscard]] const std::string &getName() const;

    [[nodiscard]] int32_t getPort() const;

    [[nodiscard]] const std::string &getDatabaseName() const;

    [[nodiscard]] bool isDefault() const;

    friend std::ostream &operator<<(std::ostream &os,
                                    const DatabaseConnectionEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONENTRY_H
