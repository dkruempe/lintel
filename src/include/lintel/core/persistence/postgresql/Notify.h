#ifndef LINTEL_POSTGRESQL_NOTIFY_H
#define LINTEL_POSTGRESQL_NOTIFY_H

#include <libpq-fe.h>

#include <sys/select.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "Connection.h"

namespace postgresql {
    /**
     * Listens for PostgreSQL NOTIFY events on a dedicated thread.
     *
     * Uses its own libpq connection (libpq is not thread-safe), so the
     * passed-in Connection is only used to derive the connection parameters.
     */
    class Notify {
    private:
        static constexpr int32_t kTimeoutSeconds = 1;
        std::atomic<bool> m_shutdown = false;
        Connection &m_connection;
        PGconn *m_conn = nullptr;
        std::thread m_thread;

        std::string m_tableName;
        std::function<void()> m_callBack;
        struct timeval m_timeout{
                kTimeoutSeconds, 0
        };

        /** Main loop that polls for notifications. */
        void run();

        /** Issues the LISTEN command for the configured table. */
        void listen();

    public:
        /**
         * Constructs a notify listener.
         * @param connection the PostgreSQL connection
         * @param tableName the table/channel to listen on
         * @param callBack the callback to invoke on notification
         */
        explicit Notify(Connection &connection, std::string tableName,
                        std::function<void()> &callBack);

        /** Destructor, signals shutdown and joins the listener thread. */
        ~Notify();
    };
}  // namespace postgresql

#endif  // LINTEL_POSTGRESQL_NOTIFY_H
