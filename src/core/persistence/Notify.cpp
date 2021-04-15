#include "base_library/core/persistence/Notify.h"

#include <utility>

namespace db {
Notify::Notify(Connection &connection, std::function<void()> functionCallBack,
               std::string tableName)
    : connection(connection) {
  switch (connection.connectionType) {
  case SQLite:
    notifySqlite = std::make_unique<sqlite::Notify>(
        *connection.connSQLite, functionCallBack, tableName);
    break;
  case PostgreSQL:
    notifyPostgresql = std::make_unique<postgresql::Notify>(
        *connection.conn, tableName, functionCallBack);
    break;
  }
}
} // namespace db