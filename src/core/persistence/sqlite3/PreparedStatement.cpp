#include "base_library/core/persistence/sqlite3/PreparedStatement.h"

#include "base_library/core/exceptions/SQLException.h"

namespace sqlite {
PreparedStatement::PreparedStatement(Connection &connection,
                                     const std::string &statement,
                                     const std::string &statementName)
    : connection(connection),
      statement(statement),
      statementName(statementName) {
  connection.prepareStatement(statementName, statement);
}

std::shared_ptr<Result> PreparedStatement::execute(
    const std::vector<std::string> &params) {
  if (closed) {
    throw db::SQLException(
        "SQLite: prepared statement is closed => abort execute");
  }
  db::Parameters parameters(params);
  return connection.executePreparedStatement(statementName, parameters);
}

void PreparedStatement::close() {
  closed = true;
  connection.finalizePreparedStatement(statementName);
}

PreparedStatement::~PreparedStatement() {
  connection.finalizePreparedStatement(statementName);
}
}  // namespace sqlite