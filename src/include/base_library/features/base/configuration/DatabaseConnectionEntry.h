#ifndef CPP_BASE_LIBRARY_DATABASECONNECTIONENTRY_H
#define CPP_BASE_LIBRARY_DATABASECONNECTIONENTRY_H

#include <ostream>

#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/features/base/configuration/Entry.h"

/** Configuration entry for a database connection */
class DatabaseConnectionEntry : public Entry {
private:
    /** The connection string */
    const std::string m_connection;
    /** The database user name */
    const std::string m_userName;
    /** The database password */
    const std::string m_password;
    /** The database connection type */
    db::ConnectionType m_type;
    /** The connection name */
    const std::string m_name;
    /** The connection port */
    const int32_t m_port;
    /** The database name */
    const std::string m_databaseName;
    /** Whether this is the default connection */
    bool m_isDefault;

public:
    /** Construct a database connection entry
     * @param component The configuration component name
     * @param connection The connection string
     * @param userName The database user name
     * @param password The database password
     * @param type The connection type
     * @param name The connection name
     * @param port The connection port
     * @param databaseName The database name
     * @param isDefault Whether this is the default connection */
    DatabaseConnectionEntry(std::string_view component, std::string connection,
                            std::string userName, std::string password,
                            db::ConnectionType type, std::string name,
                            int32_t port, std::string databaseName,
                            bool isDefault);

    /** Get the connection string
     * @return The connection string */
    [[nodiscard]] const std::string &getConnection() const;

    /** Get the database user name
     * @return The user name */
    [[nodiscard]] const std::string &getUserName() const;

    /** Get the database password
     * @return The password */
    [[nodiscard]] const std::string &getPassword() const;

    /** Get the connection type
     * @return The connection type */
    [[nodiscard]] const db::ConnectionType &getType() const;

    /** Get the connection name
     * @return The name */
    [[nodiscard]] const std::string &getName() const;

    /** Get the connection port
     * @return The port number */
    [[nodiscard]] int32_t getPort() const;

    /** Get the database name
     * @return The database name */
    [[nodiscard]] const std::string &getDatabaseName() const;

    /** Check if this is the default connection
     * @return True if default */
    [[nodiscard]] bool isDefault() const;

    /** Stream insertion operator for database connection entries
     * @param os The output stream
     * @param entry The entry to output
     * @return The output stream */
    friend std::ostream &operator<<(std::ostream &os,
                                    const DatabaseConnectionEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_DATABASECONNECTIONENTRY_H
