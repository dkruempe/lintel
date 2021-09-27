#ifndef CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H
#define CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/ParameterBuilder.h"
#include "base_library/core/persistence/Result.h"
#include "base_library/core/persistence/postgresql/PreparedStatement.h"
#include "base_library/core/persistence/sqlite3/PreparedStatement.h"

namespace db {
class PreparedStatement {
 private:
  const Connection &m_connection;
  std::unique_ptr<postgresql::PreparedStatement> m_preparedStatement = nullptr;
  std::unique_ptr<sqlite::PreparedStatement> m_preparedStatementSQLite =
      nullptr;

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

  Result execute(const ParameterBuilder &builder);

  void close();
};
}  // namespace db

#endif  // CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H
