#include "base_library/core/persistence/postgresql/Notify.h"

#include <string.h>

#include <iostream>

namespace postgresql {
    Notify::Notify(Connection &connection, std::string tableName,
                   std::function<void()> &callBack)
            : m_connection(connection),
              m_thread([&]() { run(); }),
              m_tableName(std::move(tableName)),
              m_callBack(callBack) {}

    void Notify::listen() {
        if (m_shutdown) {
            return;
        }

        auto result = m_connection.execute("LISTEN " + m_tableName);
        if (!result->isState(ExecStatusType::PGRES_COMMAND_OK)) {
            throw db::SQLException("LISTEN command failed: " +
                                   m_connection.getErrorMessage());
        }
        int sock = PQsocket(m_connection.m_conn);

        if (sock < 0) {
            throw db::SQLException("LISTEN sock connection failed");
        }
        fd_set input_mask;
        FD_ZERO(&input_mask);
        FD_SET(sock, &input_mask);
        const int rc = select(sock + 1, &input_mask, nullptr, nullptr, &m_timeout);
        switch (rc) {
            // error happen
            case -1:
                throw db::SQLException("LISTEN select() failed: " +
                                       std::string(strerror(errno)));
                break;
            case 0:
                // timeout
                break;
            default:
                PQconsumeInput(m_connection.m_conn);
                PGnotify *notify = nullptr;
                do {
                    notify = PQnotifies(m_connection.m_conn);
                    // clean received messages
                    if (notify != nullptr) {
                        PQfreemem(notify);
                    }
                } while (notify != nullptr);
                m_callBack();
                // worked
                break;
        }
    }

    void Notify::run() {
        while (!m_shutdown) {
            listen();
        }
    }

    Notify::~Notify() {
        m_shutdown.store(true);
        m_thread.join();
    }
}  // namespace postgresql