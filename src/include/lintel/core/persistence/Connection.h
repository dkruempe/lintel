#ifndef LINTEL_CONNECTION_H
#define LINTEL_CONNECTION_H

#include <memory>

#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/persistence/postgresql/Connection.h"
#include "lintel/core/persistence/sqlite3/Connection.h"

namespace lintel {
namespace db {

  class Transaction;

  class Statement;

  class PreparedStatement;

  class Notify;

  /**
   * Unified database connection wrapping either a PostgreSQL or SQLite connection.
   */
  class Connection
  {
  private:
    std::shared_ptr<postgresql::Connection> m_conn = nullptr;
    std::shared_ptr<sqlite::Connection> m_connSQLite = nullptr;

    ConnectionType m_connectionType;

    friend class Transaction;

    friend class Statement;

    friend class PreparedStatement;

    friend class Notify;

    friend class Cursor;

  public:
    /**
     * Constructs a connection from a type and connection info string.
     * @param connectionType the database type (PostgreSQL or SQLite)
     * @param connectionInfo connection string or file path
     */
    explicit Connection(ConnectionType connectionType, const std::string &connectionInfo);

    /**
     * Constructs a connection from a database connection configuration entry.
     * @param connectionEntry the configuration entry describing the connection
     */
    explicit Connection(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

    explicit Connection(Connection &connection) = delete;

    /**
     * Reports whether the underlying driver still considers the connection usable.
     *
     * PostgreSQL asks `PQstatus`; SQLite runs a lightweight `SELECT 1` probe
     * (the recent error code is not a reliable health signal, because it stays
     * set after a non-fatal error such as a constraint violation). Used by
     * @ref lintel::db::ConnectionPool to drop broken connections on release.
     * @return true when the connection may be reused */
    [[nodiscard]] bool isHealthy() const;
  };
}// namespace db

}// namespace lintel
#endif// LINTEL_CONNECTION_H
