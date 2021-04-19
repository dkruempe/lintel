#ifndef CPP_BASE_LIBRARY_SQLITE_PREPAREDSTATEMENT_H
#define CPP_BASE_LIBRARY_SQLITE_PREPAREDSTATEMENT_H

#include "Connection.h"

namespace sqlite {
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
  Connection &connection;
  const std::string statement;
  const std::string statementName;

 public:
  /**
   *
   * @param connection
   * @param statement
   */
  PreparedStatement(Connection &connection, const std::string &statement,
                    const std::string &statementName);

  ~PreparedStatement();

  std::shared_ptr<Result> execute(const std::vector<std::string> &params);

  void close();
};
}  // namespace sqlite
#endif  // CPP_BASE_LIBRARY_SQLITE_PREPAREDSTATEMENT_H
