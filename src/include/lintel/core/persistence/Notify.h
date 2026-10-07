#ifndef LINTEL_NOTIFY_H
#define LINTEL_NOTIFY_H

#include <memory>

#include "Connection.h"
#include "lintel/core/persistence/postgresql/Notify.h"
#include "lintel/core/persistence/sqlite3/Notify.h"

namespace db {
    /**
     * Unified database notification listener wrapping PostgreSQL LISTEN/NOTIFY
     * or SQLite update hooks.
     */
    class Notify {
    private:
        Connection &m_connection;
        std::unique_ptr<sqlite::Notify> m_notifySqlite = nullptr;
        std::unique_ptr<postgresql::Notify> m_notifyPostgresql = nullptr;

    public:
        /**
         * Constructs a notification listener.
         * @param connection the database connection
         * @param functionCallBack callback invoked when a notification is received
         * @param tableName the table to watch for changes
         */
        explicit Notify(Connection &connection,
                        std::function<void()> functionCallBack,
                        std::string tableName);
    };
}  // namespace db

#endif  // LINTEL_NOTIFY_H
