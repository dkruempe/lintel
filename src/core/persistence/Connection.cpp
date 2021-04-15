#include "base_library/core/persistence/Connection.h"

db::Connection::Connection(ConnectionType connectionType,
                           const std::string &connectionInfo)
    : connectionType(connectionType) {
  switch (connectionType) {
  case ConnectionType::SQLite:
    connSQLite = std::make_shared<sqlite::Connection>(connectionInfo);
    break;
  case ConnectionType::PostgreSQL:
    conn = std::make_shared<postgresql::Connection>(connectionInfo);
    break;
  default:
    break;
  }
}
