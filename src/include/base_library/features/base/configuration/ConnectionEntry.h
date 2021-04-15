#ifndef CPP_BASE_LIBRARY_CONNECTIONENTRY_H
#define CPP_BASE_LIBRARY_CONNECTIONENTRY_H

#include "Entry.h"
#include <ostream>

class ConnectionEntry : public Entry {
private:
  const std::string connection;
  const std::string userName;
  const std::string password;
  const std::string type;

public:
  ConnectionEntry(std::string_view component, std::string connection,
                  std::string userName, std::string password, std::string type);

  [[nodiscard]] const std::string &getConnection() const;
  [[nodiscard]] const std::string &getUserName() const;
  [[nodiscard]] const std::string &getPassword() const;
  [[nodiscard]] const std::string &getType() const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const ConnectionEntry &entry);
};

#endif // CPP_BASE_LIBRARY_CONNECTIONENTRY_H
