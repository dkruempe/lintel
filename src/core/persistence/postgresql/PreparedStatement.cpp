#include "base_library/core/persistence/postgresql/PreparedStatement.h"
#include "base_library/core/persistence/Parameter.h"

#include <algorithm>

namespace postgresql {
int32_t PreparedStatement::initNParams(const std::string &tempStatement) {
  return std::count_if(tempStatement.begin(), tempStatement.end(),
                       [](char temp) { return temp == '?'; });
}

std::string PreparedStatement::initStatement(const std::string &tempStatement) {
  std::string temp;
  int32_t counter = 0;
  for (const char &iter : tempStatement) {
    if (iter == '?') {
      temp += "$" + std::to_string(++counter);
    } else {
      temp += iter;
    }
  }
  return temp;
}

PreparedStatement::PreparedStatement(const Connection &connection,
                                     const std::string &statement,
                                     const std::string &statementName)
    : connection(connection), nParams(initNParams(statement)),
      statementName(statementName), statement(initStatement(statement)) {
  std::shared_ptr<Result> result = connection.prepareStatement(
      this->statementName, this->statement, nParams);
  if (!result->isState(PGRES_COMMAND_OK)) {
    throw db::SQLException("Prepare of statement '" + statementName +
                           "' failed: " + connection.getErrorMessage());
  }
}

std::shared_ptr<Result>
PreparedStatement::execute(const std::vector<std::string> &params) {
  if (closed) {
    throw db::SQLException("Prepared Statement '" + statementName + "' closed");
  }

  std::shared_ptr<Result> result = connection.executePreparedStatement(
      statementName, statement, nParams, db::Parameters(params));
  if (!result->isState(PGRES_TUPLES_OK) && !result->isState(PGRES_COMMAND_OK)) {
    throw db::SQLException("Prepared execution failed: " +
                           connection.getErrorMessage());
  }
  return result;
}

void PreparedStatement::close() {
  if (closed) {
    return;
  }
  std::shared_ptr<Result> result =
      connection.execute("DEALLOCATE " + statementName);
  if (!result->isState(PGRES_COMMAND_OK)) {
    throw db::SQLException("Closing of prepared statement failed: " +
                           connection.getErrorMessage());
  }
  closed = true;
}
} // namespace postgresql