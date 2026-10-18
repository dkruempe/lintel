#ifndef LINTEL_DATABASECONNECTIONENTRY_H
#define LINTEL_DATABASECONNECTIONENTRY_H

#include <ostream>

#include "lintel/core/configuration/Entry.h"
#include "lintel/core/persistence/ConnectionType.h"

namespace lintel {
/** Configuration entry for a database connection */
class DatabaseConnectionEntry : public Entry
{
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
  /** Maximum number of pooled connections (0 = pooling disabled) */
  int32_t m_poolSize;
  /** How long an acquire waits for a free pooled connection, in milliseconds */
  int32_t m_poolAcquireTimeoutMs;

public:
  /** Default pool size for PostgreSQL connections (parallel HTTP threads). */
  static constexpr int32_t kDefaultPostgresPoolSize = 4;
  /** Default pool size for SQLite connections (a single serialized connection). */
  static constexpr int32_t kDefaultSqlitePoolSize = 1;
  /** Default wait time of a pool acquire, in milliseconds. */
  static constexpr int32_t kDefaultPoolAcquireTimeoutMs = 5000;

  /** Construct a database connection entry
   * @param component The configuration component name
   * @param connection The connection string
   * @param userName The database user name
   * @param password The database password
   * @param type The connection type
   * @param name The connection name
   * @param port The connection port
   * @param databaseName The database name
   * @param isDefault Whether this is the default connection
   * @param poolSize Maximum pooled connections; a negative value selects the
   *        type default (PostgreSQL 4, SQLite 1), 0 disables pooling
   * @param poolAcquireTimeoutMs Acquire timeout in milliseconds; a negative
   *        value selects the default (5000) */
  DatabaseConnectionEntry(std::string_view component,
    std::string connection,
    std::string userName,
    std::string password,
    db::ConnectionType type,
    std::string name,
    int32_t port,
    std::string databaseName,
    bool isDefault,
    int32_t poolSize = -1,
    int32_t poolAcquireTimeoutMs = -1);

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
  [[nodiscard]] constexpr int32_t getPort() const { return m_port; }

  /** Get the database name
   * @return The database name */
  [[nodiscard]] const std::string &getDatabaseName() const;

  /** Check if this is the default connection
   * @return True if default */
  [[nodiscard]] constexpr bool isDefault() const { return m_isDefault; }

  /** Get the maximum number of pooled connections
   * @return The pool size; 0 disables pooling */
  [[nodiscard]] constexpr int32_t getPoolSize() const { return m_poolSize; }

  /** Get the pool acquire timeout
   * @return The acquire timeout in milliseconds */
  [[nodiscard]] constexpr int32_t getPoolAcquireTimeoutMs() const { return m_poolAcquireTimeoutMs; }

  /** Stream insertion operator for database connection entries
   * @param os The output stream
   * @param entry The entry to output
   * @return The output stream */
  friend std::ostream &operator<<(std::ostream &os, const DatabaseConnectionEntry &entry);
};

}// namespace lintel
#endif// LINTEL_DATABASECONNECTIONENTRY_H
