#include "base_library/persistence/PreparedStatement.h"
#include "base_library/persistence/Result.h"
#include "base_library/persistence/postgresql/PreparedStatement.h"

namespace db {
PreparedStatement::PreparedStatement(const Connection &connection,
                                     const std::string &statement,
                                     const std::string &statementName)
    : connection(connection) {
  switch (connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    preparedStatement = std::make_unique<postgresql::PreparedStatement>(
        *connection.conn, statement, statementName);
    break;
  }
}

db::Result PreparedStatement::execute(const std::vector<std::string> &params) {
  switch (connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    preparedStatement->execute(params);
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
  case SQLite:
    break;
  case PostgreSQL:
    preparedStatement->close();
    break;
  }
}
} // namespace db