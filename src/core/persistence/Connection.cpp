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

db::Connection::Connection(
    const std::shared_ptr<ConnectionEntry> &connectionEntry)
    : connectionType(connectionEntry->getType()) {
  switch (connectionType) {
    case ConnectionType::SQLite:
      connSQLite = std::make_shared<sqlite::Connection>(connectionEntry);
      break;
    case ConnectionType::PostgreSQL:
      conn = std::make_shared<postgresql::Connection>(connectionEntry);
      break;
    default:
      break;
  }
}
