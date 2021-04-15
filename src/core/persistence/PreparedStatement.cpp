#include "base_library/core/persistence/PreparedStatement.h"
#include "base_library/core/persistence/Result.h"
#include "base_library/core/persistence/postgresql/PreparedStatement.h"

namespace db {
PreparedStatement::PreparedStatement(const Connection &connection,
                                     const std::string &statement,
                                     const std::string &statementName)
    : connection(connection) {
  switch (connection.connectionType) {
  case ConnectionType::SQLite:
    preparedStatementSQLite = std::make_unique<sqlite::PreparedStatement>(
        *connection.connSQLite, statement, statementName);
    break;
  case ConnectionType::PostgreSQL:
    preparedStatement = std::make_unique<postgresql::PreparedStatement>(
        *connection.conn, statement, statementName);
    break;
  case ConnectionType::UNDEFINED:
    throw db::SQLException("Undefined Database Type");
  }
}

db::Result PreparedStatement::execute(const std::vector<std::string> &params) {
  switch (connection.connectionType) {
  case ConnectionType::SQLite:
    preparedStatementSQLite->execute(params);
    break;
  case ConnectionType::PostgreSQL:
    preparedStatement->execute(params);
    break;
  case ConnectionType::UNDEFINED:
    throw db::SQLException("Undefined Database Type");
  }
  return Result();
}

int32_t PreparedStatement::initNParams(const std::string &tempStatement) {
  return postgresql::PreparedStatement::initNParams(tempStatement);
}
std::string PreparedStatement::initStatement(const std::string &tempStatement) {
  return postgresql::PreparedStatement::initStatement(tempStatement);
}

void PreparedStatement::close() {
  switch (connection.connectionType) {
  case ConnectionType::SQLite:
    preparedStatementSQLite->close();
    break;
  case ConnectionType::PostgreSQL:
    preparedStatement->close();
    break;
  case ConnectionType::UNDEFINED:
    throw db::SQLException("Undefined Database Type");
  }
}
} // namespace db