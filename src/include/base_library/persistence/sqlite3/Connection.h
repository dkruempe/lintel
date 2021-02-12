#ifndef CPP_BASE_LIBRARY_SQLITE_CONNECTION_H
#define CPP_BASE_LIBRARY_SQLITE_CONNECTION_H

#include <map>
#include <sqlite3.h>
#include <string>

#include "base_library/persistence/Parameter.h"
#include "base_library/persistence/sqlite3/Result.h"

namespace sqlite {
class Transaction;
class Statement;
class PreparedStatement;
class Notify;

class Connection {
private:
  sqlite3 *db;
  friend class Transaction;
  friend class Statement;
  friend class PreparedStatement;
  friend class Notify;

  std::map<std::string, sqlite3_stmt *> preparedStatements;

private:
  [[nodiscard]] std::shared_ptr<Result>
  execute(const std::string &statement) const;

  [[nodiscard]] std::shared_ptr<Result>
  executeParameters(const std::string &statement,
                    const db::Parameters &parameters);

  std::shared_ptr<Result> prepareStatement(const std::string &queryName,
                                           const std::string &query);

  [[nodiscard]] std::shared_ptr<Result>
  executePreparedStatement(const std::string &queryName,
                           const db::Parameters &parameters);

  void finalizePreparedStatement(const std::string &queryName);

  static int callBack(void *a_param, int argc, char **argv, char **column);

  [[nodiscard]] std::string getErrorMessage() const;

public:
  explicit Connection(const std::string &connectionInfo);

  Connection(Connection &connection) = delete;

  ~Connection();
};
} // namespace sqlite

#endif // CPP_BASE_LIBRARY_SQLITE_CONNECTION_H
