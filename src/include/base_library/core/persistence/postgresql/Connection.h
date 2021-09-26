#ifndef CPP_BASE_LIBRARY_POSTGRES_CONNECTION_H
#define CPP_BASE_LIBRARY_POSTGRES_CONNECTION_H

#include <libpq-fe.h>

#include <memory>
#include <string>
#include <vector>

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/core/persistence/Parameter.h"
#include "base_library/core/persistence/postgresql/Result.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

namespace postgresql {
class Transaction;
class Statement;
class PreparedStatement;
class Notify;

class Connection {
 private:
  PGconn *m_conn;
  friend class Transaction;
  friend class Statement;
  friend class PreparedStatement;
  friend class Notify;

  [[nodiscard]] std::shared_ptr<Result> execute(
      const std::string &statement) const;

  [[nodiscard]] std::shared_ptr<Result> executeParameters(
      const std::string &statement, const db::Parameters &parameters) const;

  [[nodiscard]] std::shared_ptr<Result> prepareStatement(
      const std::string &statementName, const std::string &query,
      int32_t nParams) const;

  [[nodiscard]] std::shared_ptr<Result> executePreparedStatement(
      const std::string &statementName, const std::string &query,
      int32_t nParams, const db::Parameters &parameters) const;

  [[nodiscard]] std::string getErrorMessage() const;

 public:
  explicit Connection(const std::string &connectionInfo);

  explicit Connection(const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

  Connection(Connection &connection) = delete;

  ~Connection();
};
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRES_CONNECTION_H
