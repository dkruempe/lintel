#ifndef LINTEL_CONNECTIONPOOL_H
#define LINTEL_CONNECTIONPOOL_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <thread>
#include <vector>

#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/persistence/Connection.h"

namespace lintel {
namespace db {
  /**
   * Tuning of a @ref ConnectionPool.
   *
   * `maxConnections` encodes three operating modes with one implementation:
   * | value | behaviour |
   * |---|---|
   * | `0` | no pool; `acquire()` fails, the registry keeps no entry |
   * | `1` | one persistent connection, serialized between callers (SQLite default) |
   * | `N>1` | bounded pool (PostgreSQL default, parallel HTTP threads) |
   */
  struct PoolSettings
  {
    /** Maximum number of connections held by the pool (0 = pooling disabled). */
    std::size_t maxConnections = 0;
    /** Time @ref ConnectionPool::acquire waits for a free connection before it throws. */
    std::chrono::milliseconds acquireTimeout{ 5000 };
  };

  /**
   * A bounded pool of database connections handed out through RAII leases.
   *
   * The pool opens a TCP connection (connect, auth, optional TLS) once per slot
   * instead of once per call and hands the slot out through a @ref Lease. All
   * operations are thread-safe; a caller that has to wait longer than
   * @ref PoolSettings::acquireTimeout gets a @ref PoolTimeoutException that
   * lists the outstanding leases.
   *
   * Re-entrancy is detected: a thread that already holds a lease of this pool
   * and calls `acquire()` again fails immediately instead of waiting for the
   * timeout. This is the typical `poolSize=1` + nested-call deadlock, which an
   * unbounded wait would only surface much later.
   *
   * On release the connection is probed by @ref Connection::isHealthy (see the
   * injectable @ref HealthCheck); a broken connection is closed and dropped, so
   * the next `acquire()` builds a fresh one instead of handing out a dead
   * handle.
   *
   * State leaks across reuse are the caller's responsibility: a pooled
   * connection keeps its session state (temporary tables, `SET`,
   * prepared statements), so code that runs on a lease must not rely on
   * connection-local state surviving a release. See the *Connection pooling*
   * section of `docs/architecture.dox`.
   */
  class ConnectionPool
  {
  public:
    /** Creates a fresh connection for the pool. Defaults to a connection built from the entry. */
    using Connector = std::function<std::unique_ptr<Connection>()>;
    /** Reports whether a connection may return to the pool. Injectable for tests. */
    using HealthCheck = std::function<bool(const Connection &)>;

    /**
     * RAII handle on a pooled connection.
     *
     * Movable but not copyable: a lease belongs to the scope that acquired it
     * and returns the connection on destruction (also while an exception
     * unwinds). A lease that wanders out of its scope is a programming error,
     * which is why copying is deleted; `acquire()` returns the lease by value
     * through guaranteed copy elision.
     */
    class Lease
    {
    public:
      /** Construct an empty lease that holds no connection. */
      Lease() = default;
      Lease(const Lease &) = delete;
      Lease &operator=(const Lease &) = delete;
      /** @param other lease to move from; @p other is emptied */
      Lease(Lease &&other) noexcept;
      /** @param other lease to move from; a held connection is released first
       *  @return this lease */
      Lease &operator=(Lease &&other) noexcept;
      /** Release the held connection back to the pool. */
      ~Lease();

      /** @return the leased connection
       *  @throws SQLException if the lease is empty (moved-from or default-constructed) */
      [[nodiscard]] Connection &connection() const;

      /** @return true if the lease currently owns a connection */
      [[nodiscard]] bool valid() const { return m_connection != nullptr; }

    private:
      friend class ConnectionPool;

      /**
       * Take ownership of an acquired connection.
       * @param pool pool the connection returns to
       * @param connection the acquired connection
       * @param owner thread that acquired the lease
       */
      Lease(ConnectionPool *pool, std::unique_ptr<Connection> connection, std::thread::id owner);

      /** Return the held connection (if any) to the pool and clear the lease. */
      void reset();

      ConnectionPool *m_pool = nullptr;
      std::unique_ptr<Connection> m_connection;
      std::thread::id m_owner;
    };

    /**
     * Construct a pool.
     * @param connectionEntry connection definition the pool connects with
     * @param settings pool size and acquire timeout
     * @param healthCheck probe run before a connection returns to the pool;
     *        defaults to @ref Connection::isHealthy (injectable for tests)
     * @param connector factory for fresh connections; defaults to a connection
     *        built from @p connectionEntry (injectable for tests)
     */
    explicit ConnectionPool(std::shared_ptr<DatabaseConnectionEntry> connectionEntry,
      PoolSettings settings,
      HealthCheck healthCheck = {},
      Connector connector = {});

    ConnectionPool(const ConnectionPool &) = delete;
    ConnectionPool &operator=(const ConnectionPool &) = delete;

    /**
     * Acquire a connection, waiting up to @ref PoolSettings::acquireTimeout.
     * @param location caller location, recorded for diagnostics
     * @return a lease owning the connection
     * @throws PoolTimeoutException on timeout, on re-entrancy (the calling
     *         thread already holds a lease of this pool) or when pooling is
     *         disabled (`maxConnections == 0`) */
    Lease acquire(std::source_location location = std::source_location::current());

    /** @return the configured connection name */
    [[nodiscard]] const std::string &name() const;

    /** @return the configured maximum number of connections */
    [[nodiscard]] std::size_t maxConnections() const { return m_settings.maxConnections; }

    /** @return number of connections currently handed out (for tests and diagnostics) */
    [[nodiscard]] std::size_t outstanding() const;

    /** @return number of idle connections parked in the pool (for tests and diagnostics) */
    [[nodiscard]] std::size_t available() const;

  private:
    friend class Lease;

    /**
     * Return a connection to the pool after its health check.
     * @param connection connection to return, may be null
     * @param owner thread that acquired the lease
     */
    void release(std::unique_ptr<Connection> connection, std::thread::id owner);

    /** @param connection connection to probe
     *  @return true when the connection may be reused */
    [[nodiscard]] bool isHealthy(const Connection &connection) const;

    /** @param waiting thread that is about to time out
     *  @return human readable list of every outstanding lease (caller holds the mutex) */
    [[nodiscard]] std::string describeOutstanding(std::thread::id waiting) const;

    /** @param location a captured source location
     *  @return `function (file:line)` */
    [[nodiscard]] static std::string describeLocation(const std::source_location &location);

    /** Outstanding leases, keyed by the acquiring thread. */
    struct OutstandingLease
    {
      /** thread that holds the lease */
      std::thread::id m_owner;
      /** where the lease was acquired */
      std::source_location m_location;
      /** when the lease was acquired */
      std::chrono::steady_clock::time_point m_acquired;
    };

    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    PoolSettings m_settings;
    HealthCheck m_healthCheck;
    Connector m_connector;
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::vector<std::unique_ptr<Connection>> m_idle;
    std::map<std::thread::id, OutstandingLease> m_outstanding;
    std::size_t m_created = 0;
  };
}// namespace db

}// namespace lintel
#endif// LINTEL_CONNECTIONPOOL_H
