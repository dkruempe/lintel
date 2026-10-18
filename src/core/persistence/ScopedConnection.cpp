#include "lintel/core/persistence/ScopedConnection.h"

#include <utility>

namespace lintel {
namespace db {
  ScopedConnection::ScopedConnection(std::shared_ptr<DatabaseConnectionEntry> connectionEntry)
    : m_connectionEntry(std::move(connectionEntry))
  {
    if (m_connectionEntry == nullptr) {
      // Should not happen as per existing code patterns, but be safe.
      throw SQLException("ScopedConnection: null connection entry");
    }
    const auto &name = m_connectionEntry->getName();
    if (ConnectionPoolRegistry::instance().hasPool(name)) {
      m_lease = ConnectionPoolRegistry::instance().poolOf(name)->acquire();
      m_isPooled = true;
    } else {
      m_directConnection = std::make_unique<Connection>(m_connectionEntry);
      m_isPooled = false;
    }
  }

  ScopedConnection::~ScopedConnection() { reset(); }

  ScopedConnection::ScopedConnection(ScopedConnection &&other) noexcept
    : m_connectionEntry(std::move(other.m_connectionEntry)), m_isPooled(other.m_isPooled),
      m_lease(std::move(other.m_lease)), m_directConnection(std::move(other.m_directConnection))
  {
    other.m_isPooled = false;
  }

  ScopedConnection &ScopedConnection::operator=(ScopedConnection &&other) noexcept
  {
    if (this != &other) {
      reset();
      m_connectionEntry = std::move(other.m_connectionEntry);
      m_isPooled = other.m_isPooled;
      m_lease = std::move(other.m_lease);
      m_directConnection = std::move(other.m_directConnection);
      other.m_isPooled = false;
    }
    return *this;
  }

  void ScopedConnection::reset()
  {
    m_lease = ConnectionPool::Lease{};
    m_directConnection.reset();
    m_connectionEntry.reset();
  }

  Connection &ScopedConnection::connection() const
  {
    if (m_isPooled) { return m_lease.connection(); }
    if (m_directConnection != nullptr) { return *m_directConnection; }
    throw SQLException("ScopedConnection: no connection held (moved-from or default-constructed)");
  }

  bool ScopedConnection::valid() const
  {
    if (m_isPooled) { return m_lease.valid(); }
    return m_directConnection != nullptr;
  }

  bool ScopedConnection::isPooled() const { return m_isPooled; }
}// namespace db

}// namespace lintel
