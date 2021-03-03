#include "base_library/configuration/ConnectionEntry.h"

#include <utility>
const std::string &ConnectionEntry::getConnection() const { return connection; }

const std::string &ConnectionEntry::getUserName() const { return userName; }

const std::string &ConnectionEntry::getPassword() const { return password; }

const std::string &ConnectionEntry::getType() const { return type; }
std::ostream &operator<<(std::ostream &os, const ConnectionEntry &entry) {
  os << static_cast<const Entry &>(entry) << " connection: " << entry.connection
     << " userName: " << entry.userName << " password: " << entry.password
     << " type: " << entry.type;
  return os;
}
ConnectionEntry::ConnectionEntry(std::string_view component, std::string connection,
                             std::string userName, std::string password,
                             std::string type)
    : Entry(component), connection(std::move(connection)),
      userName(std::move(userName)), password(std::move(password)),
      type(std::move(type)) {}
