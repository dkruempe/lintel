#include "base_library/core/persistence/Notify.h"

#include <utility>

namespace db {
Notify::Notify(Connection &connection, std::function<void()> functionCallBack,
               std::string tableName)
    : connection(connection) {
  switch (connection.connectionType) {
    case ConnectionType::SQLite:
      notifySqlite = std::make_unique<sqlite::Notify>(
          *connection.connSQLite, functionCallBack, tableName);
      break;
    case ConnectionType::PostgreSQL:
      notifyPostgresql = std::make_unique<postgresql::Notify>(
          *connection.conn, tableName, functionCallBack);
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}
}  // namespace db