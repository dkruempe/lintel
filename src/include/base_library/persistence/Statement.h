#ifndef CPP_BASE_LIBRARY_STATEMENT_H
#define CPP_BASE_LIBRARY_STATEMENT_H
#include "base_library/persistence/Connection.h"
#include "base_library/persistence/Result.h"
#include "base_library/persistence/postgresql/Statement.h"
#include "base_library/persistence/sqlite3/Statement.h"
namespace db {
class Statement {
private:
  std::unique_ptr<postgresql::Statement> statement = nullptr;
  std::unique_ptr<sqlite::Statement> statementSQLite = nullptr;
  const Connection &connection;

public:
  explicit Statement(const Connection &connection);

  Result execute(const std::string &query);

  Result execute(const std::string &query,
                 const std::vector<std::string> &params);
};
} // namespace db

#endif // CPP_BASE_LIBRARY_STATEMENT_H
