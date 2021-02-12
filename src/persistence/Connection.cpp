#include "base_library/persistence/Connection.h"

db::Connection::Connection(ConnectionType connectionType,
                           const std::string &connectionInfo)
    : connectionType(connectionType) {
  switch (connectionType) {
  case SQLite:
    connSQLite = std::make_shared<sqlite::Connection>(connectionInfo);
    break;
  case PostgreSQL:
    conn = std::make_shared<postgresql::Connection>(connectionInfo);
    break;
  default:
    break;
  }
}
