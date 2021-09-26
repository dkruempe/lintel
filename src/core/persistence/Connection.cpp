#include "base_library/core/persistence/Connection.h"

db::Connection::Connection(ConnectionType connectionType,
                           const std::string &connectionInfo)
    : m_connectionType(connectionType) {
  switch (connectionType) {
    case ConnectionType::SQLite:
      m_connSQLite = std::make_shared<sqlite::Connection>(connectionInfo);
      break;
    case ConnectionType::PostgreSQL:
      m_conn = std::make_shared<postgresql::Connection>(connectionInfo);
      break;
    default:
      break;
  }
}

db::Connection::Connection(
    const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry)
    : m_connectionType(connectionEntry->getType()) {
  switch (m_connectionType) {
    case ConnectionType::SQLite:
      m_connSQLite = std::make_shared<sqlite::Connection>(connectionEntry);
      break;
    case ConnectionType::PostgreSQL:
      m_conn = std::make_shared<postgresql::Connection>(connectionEntry);
      break;
    default:
      break;
  }
}
