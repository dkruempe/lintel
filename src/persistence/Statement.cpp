#include "base_library/persistence/Statement.h"

#include "base_library/persistence/postgresql/Statement.h"

namespace db {
Statement::Statement(const Connection &connection) : connection(connection) {
  switch (connection.connectionType) {
  case SQLite:
    statementSQLite =
        std::make_unique<sqlite::Statement>(*connection.connSQLite);
    break;
  case PostgreSQL:
    statement = std::make_unique<postgresql::Statement>(*connection.conn);
    break;
  }
}

Result Statement::execute(const std::string &query) {
  switch (connection.connectionType) {
  case SQLite:
    return Result(statementSQLite->execute(query));
    break;
  case PostgreSQL:
    return Result(statement->execute(query));
  }
  return Result();
}
Result Statement::execute(const std::string &query,
                          const std::vector<std::string> &params) {
  switch (connection.connectionType) {

  case SQLite:
    return Result(statementSQLite->execute(query, params));
    break;
  case PostgreSQL:
    return Result(statement->execute(query, params));
  }
  return Result();
}
} // namespace db