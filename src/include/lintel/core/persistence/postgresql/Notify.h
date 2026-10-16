#ifndef LINTEL_POSTGRESQL_NOTIFY_H
#define LINTEL_POSTGRESQL_NOTIFY_H

#include <libpq-fe.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include "Connection.h"

namespace lintel {
namespace postgresql {
  /**
   * Listens for PostgreSQL NOTIFY events on a dedicated thread.
   *
   * Uses its own libpq connection (libpq is not thread-safe), so the
   * passed-in Connection is only used to derive the connection parameters.
   */
  class Notify
  {
  private:
    static constexpr int32_t kTimeoutSeconds = 1;
    std::atomic<bool> m_shutdown = false;
    Connection &m_connection;
    PGconn *m_conn = nullptr;
    std::thread m_thread;

    std::string m_tableName;
    std::function<void()> m_callBack;
    /** Poll timeout for the listener select() loop. */
    std::chrono::seconds m_timeout{ kTimeoutSeconds };

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
    explicit Notify(Connection &connection, std::string tableName, std::function<void()> &callBack);

    /** Destructor, signals shutdown and joins the listener thread. */
    ~Notify();
  };
}// namespace postgresql

}// namespace lintel
#endif// LINTEL_POSTGRESQL_NOTIFY_H
