#include "base_library/core/persistence/postgresql/Notify.h"

#include <string.h>

#include <iostream>

#include "base_library/core/persistence/Identifier.h"

namespace postgresql {
    Notify::Notify(Connection &connection, std::string tableName,
                   std::function<void()> &callBack)
            : m_connection(connection),
              m_tableName(std::move(tableName)),
              m_callBack(callBack) {
        // use a dedicated connection: libpq is not thread-safe, so the
        // listener thread must not share the connection with other queries
        m_conn = PQconnectdb(m_connection.m_connInfo.c_str());
        if (m_conn == nullptr) {
            throw db::SQLException("LISTEN connection failed: PQconnectdb returned null");
        }
        if (PQstatus(m_conn) != CONNECTION_OK) {
            std::string const msg = PQerrorMessage(m_conn);
            PQfinish(m_conn);
            m_conn = nullptr;
            throw db::SQLException("LISTEN connection failed: " + msg);
        }
        // start the thread only after all members are initialized
        m_thread = std::thread([this]() { run(); });
    }

    void Notify::listen() {
        if (m_shutdown) {
            return;
        }

        // select() modifies the timeout, so reset it before each call
        m_timeout = {m_timeoutSeconds, 0};
        int sock = PQsocket(m_conn);

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
                PQconsumeInput(m_conn);
                PGnotify *notify = nullptr;
                do {
                    notify = PQnotifies(m_conn);
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
        try {
            auto result = PQexec(m_conn, ("LISTEN " + db::quoteIdentifier(m_tableName)).c_str());
            if (result == nullptr || PQresultStatus(result) != ExecStatusType::PGRES_COMMAND_OK) {
                std::string const msg = PQerrorMessage(m_conn);
                if (result != nullptr) {
                    PQclear(result);
                }
                throw db::SQLException("LISTEN command failed: " + msg);
            }
            PQclear(result);
            while (!m_shutdown) {
                listen();
            }
        } catch (const std::exception &exception) {
            std::cerr << "Notify listener stopped: " << exception.what()
                      << std::endl;
            m_shutdown.store(true);
        }
    }

    Notify::~Notify() {
        m_shutdown.store(true);
        if (m_thread.joinable()) {
            m_thread.join();
        }
        if (m_conn != nullptr) {
            PQfinish(m_conn);
            m_conn = nullptr;
        }
    }
}  // namespace postgresql
