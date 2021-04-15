#include "base_library/core/persistence/Statement.h"

#include "base_library/core/persistence/postgresql/Statement.h"

namespace db {
Statement::Statement(const Connection &connection) : connection(connection) {
  switch (connection.connectionType) {
  case ConnectionType::SQLite:
    statementSQLite =
        std::make_unique<sqlite::Statement>(*connection.connSQLite);
    break;
  case ConnectionType::PostgreSQL:
    statement = std::make_unique<postgresql::Statement>(*connection.conn);
    break;
  case ConnectionType::UNDEFINED:
    throw db::SQLException("Undefined Database Type");
  }
}

Result Statement::execute(const std::string &query) {
  switch (connection.connectionType) {
  case ConnectionType::SQLite:
    return Result(statementSQLite->execute(query));
    break;
  case ConnectionType::PostgreSQL:
    return Result(statement->execute(query));
  case ConnectionType::UNDEFINED:
    throw db::SQLException("Undefined Database Type");
  }
  return Result();
}
Result Statement::execute(const std::string &query,
                          const std::vector<std::string> &params) {
  switch (connection.connectionType) {

  case ConnectionType::SQLite:
    return Result(statementSQLite->execute(query, params));
    break;
  case ConnectionType::PostgreSQL:
    return Result(statement->execute(query, params));
  case ConnectionType::UNDEFINED:
    throw db::SQLException("Undefined Database Type");
  }
  return Result();
}
} // namespace db