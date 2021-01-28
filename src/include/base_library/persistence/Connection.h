#ifndef CPP_BASE_LIBRARY_CONNECTION_H
#define CPP_BASE_LIBRARY_CONNECTION_H
#include <memory>

#include "base_library/persistence/postgresql/Connection.h"

namespace db {
enum ConnectionType { SQLite, PostgreSQL };

class Transaction;
class Statement;
class PreparedStatement;

class Connection {
private:
  std::shared_ptr<postgresql::Connection> conn = nullptr;
  ConnectionType connectionType;
  friend class Transaction;
  friend class Statement;
  friend class PreparedStatement;
public:
  explicit Connection(ConnectionType connectionType, const std::string &connectionInfo);

  explicit Connection(Connection &connection) = delete;
};
} // namespace db

#endif // CPP_BASE_LIBRARY_CONNECTION_H
