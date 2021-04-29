#ifndef CPP_BASE_LIBRARY_POSTGRESQL_PREPAREDSTATEMENT_H
#define CPP_BASE_LIBRARY_POSTGRESQL_PREPAREDSTATEMENT_H

#include "Connection.h"

namespace postgresql {
/**
 * format of prepared statement
 * example:
 * insert into table (..,..) values(?,?);
 *
 * => ? will be replace automatically with $1 and $2
 * => algorithm will automatically count the number of parameters
 */
class PreparedStatement {
 private:
  bool m_closed = false;
  const Connection &m_connection;
  const int32_t m_nParams;
  const std::string m_statementName;
  const std::string m_statement;

 public:
  static int32_t initNParams(const std::string &tempStatement);

  static std::string initStatement(const std::string &tempStatement);

  /**
   *
   * @param connection
   * @param statement
   */
  PreparedStatement(const Connection &connection, const std::string &statement,
                    const std::string &statementName);

  ~PreparedStatement() = default;

  std::shared_ptr<Result> execute(const std::vector<std::string> &params);

  void close();
};
}  // namespace postgresql
#endif  // CPP_BASE_LIBRARY_POSTGRESQL_PREPAREDSTATEMENT_H
