#ifndef LINTEL_CONNECTIONPOOLREGISTRY_H
#define LINTEL_CONNECTIONPOOLREGISTRY_H

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "lintel/core/persistence/ConnectionPool.h"

namespace lintel {
namespace db {
  /**
   * Process-wide registry of named connection pools.
   *
   * The pattern mirrors @ref lintel::ReadinessRegistry: a process-wide singleton
   * that components from different layers (persistence plugins, features) fill
   * during startup and that the layers needing a pool read by the connection
   * name. It is deliberately *not* a DI registration: the container resolves by
   * type, not by name, so several pools could not be addressed.
   *
   * The registry never creates a pool implicitly. Only
   * `DatabaseBootstrapPlugin` registers pools, and only for a configured
   * `DatabaseConnectionEntry` with `poolSize > 0`. A process without database
   * entries (e.g. the worker using `cfg/bootstrap_worker.xml`) therefore has an
   * empty registry, and `poolOf()` throws @ref PoolNotFoundException instead of
   * falling back to a lazily created default pool.
   *
   * Thread safety: every method may be called from any thread.
   */
  class ConnectionPoolRegistry
  {
  public:
    /** @return the process-wide registry instance */
    static ConnectionPoolRegistry &instance();

    /**
     * Register a pool; an existing pool of the same name is replaced.
     * @param name name the pool is reachable under (the connection name)
     * @param pool the pool to register */
    void registerPool(std::string name, std::shared_ptr<ConnectionPool> pool);

    /**
     * Look up a pool by name.
     * @param name the connection name
     * @return the registered pool
     * @throws PoolNotFoundException if no pool is registered under @p name */
    [[nodiscard]] std::shared_ptr<ConnectionPool> poolOf(const std::string &name) const;

    /** @param name the connection name
     *  @return true when a pool is registered under @p name */
    [[nodiscard]] bool hasPool(const std::string &name) const;

    /** Remove every registered pool (used by tests). */
    void clear();

    /** @return true when no pool is registered */
    [[nodiscard]] bool empty() const;

    /** @return the number of registered pools */
    [[nodiscard]] std::size_t size() const;

    /** @return the registered names in alphabetical order (for tests and diagnostics) */
    [[nodiscard]] std::vector<std::string> names() const;

  private:
    ConnectionPoolRegistry() = default;

    mutable std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<ConnectionPool>> m_pools;
  };
}// namespace db

}// namespace lintel
#endif// LINTEL_CONNECTIONPOOLREGISTRY_H
