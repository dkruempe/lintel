#include "lintel/core/persistence/ConnectionPool.h"

#include <sstream>
#include <utility>

#include "lintel/core/exceptions/PoolTimeoutException.h"
#include "lintel/core/exceptions/SQLException.h"

namespace lintel {
namespace db {
  ConnectionPool::ConnectionPool(std::shared_ptr<DatabaseConnectionEntry> connectionEntry,
    PoolSettings settings,
    HealthCheck healthCheck,
    Connector connector)
    : m_connectionEntry(std::move(connectionEntry)), m_settings(settings), m_healthCheck(std::move(healthCheck)),
      m_connector(std::move(connector))
  {}

  const std::string &ConnectionPool::name() const { return m_connectionEntry->getName(); }

  std::size_t ConnectionPool::outstanding() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_outstanding.size();
  }

  std::size_t ConnectionPool::available() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_idle.size();
  }

  ConnectionPool::Lease ConnectionPool::acquire(std::source_location location)
  {
    const std::thread::id self = std::this_thread::get_id();
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_settings.maxConnections == 0) {
      throw PoolTimeoutException("pool '" + name() + "' is disabled (poolSize=0); no connection available");
    }

    // Re-entrancy: the calling thread already holds a lease of this pool, so no
    // wait can ever succeed. Fail fast instead of after the timeout - this is
    // the typical poolSize=1 plus nested-acquire deadlock.
    const auto held = m_outstanding.find(self);
    if (held != m_outstanding.end()) {
      throw PoolTimeoutException("pool '" + name() + "' re-entrancy: thread already holds a lease acquired at "
                                 + describeLocation(held->second.m_location)
                                 + "; maxConnections=" + std::to_string(m_settings.maxConnections)
                                 + " - a nested acquire() of the same pool cannot succeed");
    }

    const auto deadline = std::chrono::steady_clock::now() + m_settings.acquireTimeout;
    for (;;) {
      if (!m_idle.empty()) {
        std::unique_ptr<Connection> connection = std::move(m_idle.back());
        m_idle.pop_back();
        m_outstanding.emplace(self, OutstandingLease{ self, location, std::chrono::steady_clock::now() });
        return Lease(this, std::move(connection), self);
      }

      if (m_created < m_settings.maxConnections) {
        // Reserve the slot before connecting so concurrent acquirers count it too.
        ++m_created;
        lock.unlock();
        std::unique_ptr<Connection> connection;
        try {
          connection = m_connector ? m_connector() : std::make_unique<Connection>(m_connectionEntry);
        } catch (...) {
          lock.lock();
          --m_created;
          m_condition.notify_one();
          throw;
        }
        lock.lock();
        m_outstanding.emplace(self, OutstandingLease{ self, location, std::chrono::steady_clock::now() });
        return Lease(this, std::move(connection), self);
      }

      if (std::chrono::steady_clock::now() >= deadline) {
        throw PoolTimeoutException(
          "pool '" + name() + "' timed out after " + std::to_string(m_settings.acquireTimeout.count())
          + " ms waiting for a free connection; maxConnections=" + std::to_string(m_settings.maxConnections)
          + "; outstanding leases:\n" + describeOutstanding(self) + "requesting acquire at "
          + describeLocation(location));
      }

      m_condition.wait_until(lock, deadline);
    }
  }

  void ConnectionPool::release(std::unique_ptr<Connection> connection, std::thread::id owner)
  {
    // Probe outside the lock: the check may do a round trip.
    bool healthy = false;
    if (connection != nullptr) {
      try {
        healthy = isHealthy(*connection);
      } catch (...) {
        healthy = false;
      }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_outstanding.erase(owner);
    if (healthy) {
      m_idle.push_back(std::move(connection));
    } else if (m_created > 0) {
      // Broken connection: drop it and free the slot, so the next acquire rebuilds.
      --m_created;
    }
    m_condition.notify_one();
  }

  bool ConnectionPool::isHealthy(const Connection &connection) const
  {
    if (m_healthCheck) { return m_healthCheck(connection); }
    return connection.isHealthy();
  }

  std::string ConnectionPool::describeOutstanding(std::thread::id waiting) const
  {
    // caller holds m_mutex
    const auto now = std::chrono::steady_clock::now();
    std::ostringstream stream;
    if (m_outstanding.empty()) { return "  (none)\n"; }
    for (const auto &[id, lease] : m_outstanding) {
      const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - lease.m_acquired).count();
      stream << "  thread " << id << " holds since " << age << " ms at " << describeLocation(lease.m_location);
      if (id == waiting) { stream << " (requesting thread)"; }
      stream << '\n';
    }
    return stream.str();
  }

  std::string ConnectionPool::describeLocation(const std::source_location &location)
  {
    std::ostringstream stream;
    stream << (location.function_name() != nullptr ? location.function_name() : "<unknown function>") << " ("
           << (location.file_name() != nullptr ? location.file_name() : "<unknown file>") << ":" << location.line()
           << ")";
    return stream.str();
  }

  ConnectionPool::Lease::Lease(ConnectionPool *pool, std::unique_ptr<Connection> connection, std::thread::id owner)
    : m_pool(pool), m_connection(std::move(connection)), m_owner(owner)
  {}

  ConnectionPool::Lease::Lease(Lease &&other) noexcept
    : m_pool(other.m_pool), m_connection(std::move(other.m_connection)), m_owner(other.m_owner)
  {
    other.m_pool = nullptr;
  }

  ConnectionPool::Lease &ConnectionPool::Lease::operator=(Lease &&other) noexcept
  {
    if (this != &other) {
      reset();
      m_pool = other.m_pool;
      m_connection = std::move(other.m_connection);
      m_owner = other.m_owner;
      other.m_pool = nullptr;
    }
    return *this;
  }

  ConnectionPool::Lease::~Lease() { reset(); }

  void ConnectionPool::Lease::reset()
  {
    if (m_pool != nullptr && m_connection != nullptr) { m_pool->release(std::move(m_connection), m_owner); }
    m_connection.reset();
    m_pool = nullptr;
  }

  Connection &ConnectionPool::Lease::connection() const
  {
    if (m_connection == nullptr) {
      throw SQLException("empty pool lease: no connection held (moved-from or default-constructed)");
    }
    return *m_connection;
  }
}// namespace db

}// namespace lintel
