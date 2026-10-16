#include "lintel/core/persistence/postgresql/Notify.h"

#include <cerrno>
#include <cstring>

#if defined(_WIN32)
// winsock2.h must be included before any <windows.h>; it provides timeval,
// fd_set, FD_ZERO/FD_SET and select() on Windows.
#include <winsock2.h>
#else
#include <sys/select.h>
#endif// _WIN32

#include <iostream>

#include "lintel/core/persistence/Identifier.h"

namespace lintel {
namespace postgresql {
  Notify::Notify(Connection &connection, std::string tableName, std::function<void()> &callBack)
    : m_connection(connection), m_tableName(std::move(tableName)), m_callBack(callBack)
  {
    // use a dedicated connection: libpq is not thread-safe, so the
    // listener thread must not share the connection with other queries
    m_conn = PQconnectdb(m_connection.m_connInfo.c_str());
    if (m_conn == nullptr) { throw db::SQLException("LISTEN connection failed: PQconnectdb returned null"); }
    if (PQstatus(m_conn) != CONNECTION_OK) {
      std::string const msg = PQerrorMessage(m_conn);
      PQfinish(m_conn);
      m_conn = nullptr;
      throw db::SQLException("LISTEN connection failed: " + msg);
    }
    // start the thread only after all members are initialized
    m_thread = std::thread([this]() { run(); });
  }

  void Notify::listen()
  {
    if (m_shutdown) { return; }

    // select() modifies the timeout, so build a fresh one before each call. The
    // intermediate long long keeps the cast portable: tv_sec is long on both
    // POSIX and Winsock, while std::chrono::seconds::rep differs per stdlib.
    timeval timeout{};
    timeout.tv_sec = static_cast<decltype(timeout.tv_sec)>(static_cast<long long>(m_timeout.count()));
    int sock = PQsocket(m_conn);

    if (sock < 0) { throw db::SQLException("LISTEN sock connection failed"); }
    fd_set input_mask;
    FD_ZERO(&input_mask);
    FD_SET(sock, &input_mask);
    const int rc = select(sock + 1, &input_mask, nullptr, nullptr, &timeout);
    switch (rc) {
    // error happen
    case -1:
      throw db::SQLException("LISTEN select() failed: " + std::string(strerror(errno)));
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
        if (notify != nullptr) { PQfreemem(notify); }
      } while (notify != nullptr);
      m_callBack();
      // worked
      break;
    }
  }

  void Notify::run()
  {
    try {
      auto result = PQexec(m_conn, ("LISTEN " + db::quoteIdentifier(m_tableName)).c_str());
      if (result == nullptr || PQresultStatus(result) != ExecStatusType::PGRES_COMMAND_OK) {
        std::string const msg = PQerrorMessage(m_conn);
        if (result != nullptr) { PQclear(result); }
        throw db::SQLException("LISTEN command failed: " + msg);
      }
      PQclear(result);
      while (!m_shutdown) { listen(); }
    } catch (const std::exception &exception) {
      std::cerr << "Notify listener stopped: " << exception.what() << '\n';
      m_shutdown.store(true);
    }
  }

  Notify::~Notify()
  {
    m_shutdown.store(true);
    if (m_thread.joinable()) { m_thread.join(); }
    if (m_conn != nullptr) {
      PQfinish(m_conn);
      m_conn = nullptr;
    }
  }
}// namespace postgresql
}// namespace lintel
