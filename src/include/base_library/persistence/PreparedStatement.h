#ifndef CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H
#define CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H

#include "Connection.h"

namespace db {
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
  bool closed = false;
  const Connection &connection;
  const int32_t nParams;
  const std::string statementName;
  const std::string statement;

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
} // namespace db
#endif // CPP_BASE_LIBRARY_PREPAREDSTATEMENT_H
