#ifndef CPP_BASE_LIBRARY_POSTGRESQL_NOTIFY_H
#define CPP_BASE_LIBRARY_POSTGRESQL_NOTIFY_H

#include <sys/select.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include "Connection.h"

namespace postgresql {
    class Notify {
    private:
        const int32_t m_timeoutSeconds = 1;
        std::atomic<bool> m_shutdown = false;
        Connection &m_connection;
        std::thread m_thread;

        std::string m_tableName;
        std::function<void()> m_callBack;
        struct timeval m_timeout{
                m_timeoutSeconds, 0
        };

        void run();

        void listen();

    public:
        explicit Notify(Connection &connection, std::string tableName,
                        std::function<void()> &callBack);

        ~Notify();
    };
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_NOTIFY_H
