#ifndef CPP_BASE_LIBRARY_CONNECTIONENTRY_H
#define CPP_BASE_LIBRARY_CONNECTIONENTRY_H

#include <ostream>

#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/features/base/configuration/Entry.h"

class ConnectionEntry : public Entry {
 private:
  const std::string connection;
  const std::string userName;
  const std::string password;
  db::ConnectionType type;
  const std::string name;
  const int32_t port;
  const std::string databaseName;

 public:
  ConnectionEntry(std::string_view component, std::string connection,
                  std::string userName, std::string password,
                  db::ConnectionType type, std::string name, int32_t port,
                  std::string databaseName);

  [[nodiscard]] const std::string &getConnection() const;
  [[nodiscard]] const std::string &getUserName() const;
  [[nodiscard]] const std::string &getPassword() const;
  [[nodiscard]] const db::ConnectionType &getType() const;
  [[nodiscard]] const std::string &getName() const;
  [[nodiscard]] int32_t getPort() const;
  [[nodiscard]] const std::string &getDatabaseName() const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const ConnectionEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_CONNECTIONENTRY_H
