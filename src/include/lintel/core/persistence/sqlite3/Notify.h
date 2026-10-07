#ifndef LINTEL_SQLITE_NOTIFY_H
#define LINTEL_SQLITE_NOTIFY_H

#include <functional>
#include <set>

#include "Connection.h"

namespace sqlite {
    /**
     * Listens for SQLite table changes via the update hook mechanism.
     */
    class Notify {
    private:
        std::string m_tableName;
        std::function<void()> m_functionCallBack;
        Connection &m_connection;

        /**
         * Static callback invoked by SQLite on table changes.
         * @param arg user data (Notify instance)
         * @param operation the type of operation (insert/update/delete)
         * @param thread database thread name
         * @param tableName the affected table name
         * @param changes row ID of the change
         */
        static void callBack(void *arg, int operation, const char *thread,
                             const char *tableName, sqlite3_int64 changes);

        /** Invokes the user callback if the changed table matches. */
        void notify();

    public:
        /**
         * Constructs a notify listener for table changes.
         * @param connection the SQLite connection
         * @param functionCallBack the callback to invoke on change
         * @param tableName the table name to watch
         */
        explicit Notify(Connection &connection,
                        std::function<void()> functionCallBack,
                        std::string tableName);

        /** Destructor, removes the update hook from the connection. */
        ~Notify();
    };
}  // namespace sqlite

#endif  // LINTEL_SQLITE_NOTIFY_H
