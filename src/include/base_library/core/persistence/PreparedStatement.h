#ifndef CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H
#define CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H

#include "Connection.h"
#include "Result.h"
#include "base_library/core/persistence/postgresql/PreparedStatement.h"
#include "base_library/core/persistence/sqlite3/PreparedStatement.h"

namespace db {
class PreparedStatement {
 private:
  const Connection &connection;
  std::unique_ptr<postgresql::PreparedStatement> preparedStatement = nullptr;
  std::unique_ptr<sqlite::PreparedStatement> preparedStatementSQLite = nullptr;

 public:
  static int32_t initNParams(const std::string &tempStatement);

  static std::string initStatement(const std::string &tempStatement);

  /**
   *
   * @param connection
   * @param statement
   */
  explicit PreparedStatement(const Connection &connection,
                             const std::string &statement,
                             const std::string &statementName);

  ~PreparedStatement() = default;

  Result execute(const std::vector<std::string> &params);

  void close();
};
}  // namespace db

#endif  // CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H
