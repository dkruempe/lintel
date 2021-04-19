#include "base_library/core/persistence/postgresql/Statement.h"

#include <algorithm>

namespace postgresql {
Statement::Statement(const Connection &connection) : connection(connection) {}

std::shared_ptr<Result> Statement::execute(const std::string &query) {
  std::shared_ptr<Result> result = connection.execute(query);
  if (!result->isState(PGRES_TUPLES_OK) && !result->isState(PGRES_COMMAND_OK)) {
    throw db::SQLException("Statement failed: " + connection.getErrorMessage());
  }
  return result;
}

int32_t Statement::initNParams(const std::string &tempStatement) {
  return std::count_if(tempStatement.begin(), tempStatement.end(),
                       [](char temp) { return temp == '?'; });
}

std::string Statement::initStatement(const std::string &tempStatement) {
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

std::shared_ptr<Result> Statement::execute(
    const std::string &query, const std::vector<std::string> &params) {
  const std::string &statement = initStatement(query);
  const int32_t nParams = initNParams(query);
  if (nParams != params.size()) {
    throw db::SQLException(
        "nParams != params.size() => check statement or parameters");
  }
  std::shared_ptr<Result> result =
      connection.executeParameters(statement, db::Parameters(params));
  if (!result->isState(PGRES_TUPLES_OK) && !result->isState(PGRES_COMMAND_OK)) {
    throw db::SQLException("Statement failed: " + connection.getErrorMessage());
  }
  return result;
}
}  // namespace postgresql