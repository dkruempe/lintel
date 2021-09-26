#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

#include <utility>
const std::string &DatabaseConnectionEntry::getConnection() const { return m_connection; }

const std::string &DatabaseConnectionEntry::getUserName() const { return m_userName; }

const std::string &DatabaseConnectionEntry::getPassword() const { return m_password; }

const std::string &DatabaseConnectionEntry::getName() const { return m_name; }

std::ostream &operator<<(std::ostream &os, const DatabaseConnectionEntry &entry) {
  os << static_cast<const Entry &>(entry) << " connection: " << entry.m_connection
     << " userName: " << entry.m_userName << " password: " << entry.m_password
     << " type: " << entry.m_type << " name: " << entry.m_name
     << " databaseName: " << entry.m_databaseName << " port: " << entry.m_port;
  return os;
}
DatabaseConnectionEntry::DatabaseConnectionEntry(std::string_view component,
                                 std::string connection, std::string userName,
                                 std::string password, db::ConnectionType type,
                                 std::string name, int32_t port,
                                 std::string databaseName)
    : Entry(component),
      m_connection(std::move(connection)),
      m_userName(std::move(userName)),
      m_password(std::move(password)),
      m_type(type),
      m_name(std::move(name)),
      m_port(port),
      m_databaseName(std::move(databaseName)) {}

const db::ConnectionType &DatabaseConnectionEntry::getType() const { return m_type; }
int32_t DatabaseConnectionEntry::getPort() const { return m_port; }
const std::string &DatabaseConnectionEntry::getDatabaseName() const {
  return m_databaseName;
}
