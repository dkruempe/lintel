#include "lintel/core/persistence/ConnectionPoolRegistry.h"

#include <utility>

#include "lintel/core/exceptions/PoolNotFoundException.h"

namespace lintel {
namespace db {
  ConnectionPoolRegistry &ConnectionPoolRegistry::instance()
  {
    static ConnectionPoolRegistry registry;
    return registry;
  }

  void ConnectionPoolRegistry::registerPool(std::string name, std::shared_ptr<ConnectionPool> pool)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pools.insert_or_assign(std::move(name), std::move(pool));
  }

  std::shared_ptr<ConnectionPool> ConnectionPoolRegistry::poolOf(const std::string &name) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto found = m_pools.find(name);
    if (found == m_pools.end()) { throw PoolNotFoundException("no connection pool registered for '" + name + "'"); }
    return found->second;
  }

  bool ConnectionPoolRegistry::hasPool(const std::string &name) const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pools.find(name) != m_pools.end();
  }

  void ConnectionPoolRegistry::clear()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pools.clear();
  }

  bool ConnectionPoolRegistry::empty() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pools.empty();
  }

  std::size_t ConnectionPoolRegistry::size() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pools.size();
  }

  std::vector<std::string> ConnectionPoolRegistry::names() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> result;
    result.reserve(m_pools.size());
    for (const auto &[name, pool] : m_pools) {
      static_cast<void>(pool);
      result.push_back(name);
    }
    return result;
  }
}// namespace db

}// namespace lintel
