#include "lintel/core/configuration/DatabaseConnectionEntry.h"

#include <utility>

namespace lintel {
namespace {
  /** @param type connection type
   *  @return the type's default pool size (PostgreSQL 4, SQLite 1, otherwise 0) */
  int32_t defaultPoolSizeOf(const db::ConnectionType &type)
  {
    switch (type.value()) {
    case db::ConnectionType::SQLite:
      return DatabaseConnectionEntry::kDefaultSqlitePoolSize;
    case db::ConnectionType::PostgreSQL:
      return DatabaseConnectionEntry::kDefaultPostgresPoolSize;
    case db::ConnectionType::UNDEFINED:
      return 0;
    }
    return 0;
  }
}// namespace

const std::string &DatabaseConnectionEntry::getConnection() const { return m_connection; }

const std::string &DatabaseConnectionEntry::getUserName() const { return m_userName; }

const std::string &DatabaseConnectionEntry::getPassword() const { return m_password; }

const std::string &DatabaseConnectionEntry::getName() const { return m_name; }

std::ostream &operator<<(std::ostream &os, const DatabaseConnectionEntry &entry)
{
  os << static_cast<const Entry &>(entry) << " connection: " << entry.m_connection << " userName: " << entry.m_userName
     << " password: [redacted]"
     << " type: " << entry.m_type << " name: " << entry.m_name << " databaseName: " << entry.m_databaseName
     << " port: " << entry.m_port;
  return os;
}

DatabaseConnectionEntry::DatabaseConnectionEntry(std::string_view component,
  std::string connection,
  std::string userName,
  std::string password,
  db::ConnectionType type,
  std::string name,
  int32_t port,
  std::string databaseName,
  bool isDefault,
  int32_t poolSize,
  int32_t poolAcquireTimeoutMs)
  : Entry(component), m_connection(std::move(connection)), m_userName(std::move(userName)),
    m_password(std::move(password)), m_type(type), m_name(std::move(name)), m_port(port),
    m_databaseName(std::move(databaseName)), m_isDefault(isDefault),
    m_poolSize(poolSize >= 0 ? poolSize : defaultPoolSizeOf(type)),
    m_poolAcquireTimeoutMs(poolAcquireTimeoutMs >= 0 ? poolAcquireTimeoutMs : kDefaultPoolAcquireTimeoutMs)
{}

const db::ConnectionType &DatabaseConnectionEntry::getType() const { return m_type; }

const std::string &DatabaseConnectionEntry::getDatabaseName() const { return m_databaseName; }
}// namespace lintel
