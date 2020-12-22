#ifndef CPP_BASE_LIBRARY_CONNECTION_H
#define CPP_BASE_LIBRARY_CONNECTION_H

#include "base_library/exceptions/SQLException.h"
#include "base_library/persistence/database/Parameter.h"
#include "base_library/persistence/database/Result.h"

#include <libpq-fe.h>
#include <string>
#include <vector>

namespace db {
class Transaction;
class Statement;
class PreparedStatement;

class Connection {
private:
  PGconn *conn;
  friend class Transaction;
  friend class Statement;
  friend class PreparedStatement;

  [[nodiscard]] Result execute(const std::string &statement) const;

  [[nodiscard]] Result executeParameters(const std::string &statement,
                                         const Parameters &parameters) const;

  [[nodiscard]] Result prepareStatement(const std::string &statementName,
                                        const std::string &query,
                                        int32_t nParams) const;

  [[nodiscard]] Result
  executePreparedStatement(const std::string &statementName,
                           const std::string &query, int32_t nParams,
                           const Parameters &parameters) const;

  [[nodiscard]] std::string getErrorMessage() const;

public:
  explicit Connection(const std::string &connectionInfo);

  Connection(Connection &connection) = delete;

  ~Connection();
};
} // namespace db

#endif // CPP_BASE_LIBRARY_CONNECTION_H
