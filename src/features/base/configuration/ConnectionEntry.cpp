#include "base_library/features/base/configuration/ConnectionEntry.h"

#include <utility>
const std::string &ConnectionEntry::getConnection() const { return connection; }

const std::string &ConnectionEntry::getUserName() const { return userName; }

const std::string &ConnectionEntry::getPassword() const { return password; }

const std::string &ConnectionEntry::getName() const { return name; }

std::ostream &operator<<(std::ostream &os, const ConnectionEntry &entry) {
  os << static_cast<const Entry &>(entry) << " connection: " << entry.connection
     << " userName: " << entry.userName << " password: " << entry.password
     << " type: " << entry.type << " name: " << entry.name
     << " databaseName: " << entry.databaseName << " port: " << entry.port;
  return os;
}
ConnectionEntry::ConnectionEntry(std::string_view component,
                                 std::string connection, std::string userName,
                                 std::string password, db::ConnectionType type,
                                 std::string name, int32_t port,
                                 std::string databaseName)
    : Entry(component),
      connection(std::move(connection)),
      userName(std::move(userName)),
      password(std::move(password)),
      type(type),
      name(std::move(name)),
      port(port),
      databaseName(std::move(databaseName)) {}

const db::ConnectionType &ConnectionEntry::getType() const { return type; }
int32_t ConnectionEntry::getPort() const { return port; }
const std::string &ConnectionEntry::getDatabaseName() const {
  return databaseName;
}
