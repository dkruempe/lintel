#ifndef LINTEL_SCOPEDCONNECTION_H
#define LINTEL_SCOPEDCONNECTION_H

#include <memory>
#include <string>

#include "lintel/core/configuration/DatabaseConnectionEntry.h"
#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ConnectionPool.h"
#include "lintel/core/persistence/ConnectionPoolRegistry.h"

namespace lintel {
namespace db {
  /**
   * RAII helper that obtains a database connection.
   *
   * - If a pool exists in the ConnectionPoolRegistry for the connection entry's
   *   name (hasPool(name) returns true), acquires a lease from poolOf(name) and
   *   exposes the leased connection.
   * - Otherwise (no pool registered; e.g. poolSize == 0 or no DB configured),
   *   opens a direct db::Connection as before. No pool is created implicitly.
   * - Exposes db::Connection &connection(). Movable but not copyable; returns
   *   the lease when destroyed (even if an exception unwinds).
   */
  class ScopedConnection
  {
  public:
    explicit ScopedConnection(std::shared_ptr<DatabaseConnectionEntry> connectionEntry);
    ~ScopedConnection();

    ScopedConnection(const ScopedConnection &) = delete;
    ScopedConnection &operator=(const ScopedConnection &) = delete;
    ScopedConnection(ScopedConnection &&other) noexcept;
    ScopedConnection &operator=(ScopedConnection &&other) noexcept;

    [[nodiscard]] Connection &connection() const;

    [[nodiscard]] bool valid() const;
    [[nodiscard]] bool isPooled() const;

  private:
    void reset();

    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;
    bool m_isPooled = false;
    ConnectionPool::Lease m_lease;
    std::unique_ptr<Connection> m_directConnection;
  };
}// namespace db

}// namespace lintel
#endif// LINTEL_SCOPEDCONNECTION_H
