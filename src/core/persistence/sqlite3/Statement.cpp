#include "base_library/core/persistence/sqlite3/Statement.h"

namespace sqlite {
Statement::Statement(Connection &connection) : connection(connection) {}

std::shared_ptr<Result> Statement::execute(const std::string &query) {
  return connection.execute(query);
}

std::shared_ptr<Result>
Statement::execute(const std::string &query,
                   const std::vector<std::string> &params) {
  return connection.executeParameters(query, db::Parameters(params));
}
} // namespace sqlite