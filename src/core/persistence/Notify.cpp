#include "base_library/core/persistence/Notify.h"

#include <utility>

namespace db {
    Notify::Notify(Connection &connection, std::function<void()> functionCallBack,
                   std::string tableName)
            : m_connection(connection) {
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                m_notifySqlite = std::make_unique<sqlite::Notify>(
                        *connection.m_connSQLite, functionCallBack, tableName);
                break;
            case ConnectionType::PostgreSQL:
                m_notifyPostgresql = std::make_unique<postgresql::Notify>(
                        *connection.m_conn, tableName, functionCallBack);
                break;
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
    }
}  // namespace db