#ifndef CPP_BASE_LIBRARY_TRANSACTION_H
#define CPP_BASE_LIBRARY_TRANSACTION_H

#include "Connection.h"
#include "base_library/core/persistence/postgresql/Transaction.h"
#include "base_library/core/persistence/sqlite3/Transaction.h"

namespace db {
class Transaction {
private:
  const Connection &connection;
  std::unique_ptr<postgresql::Transaction> transaction = nullptr;
  std::unique_ptr<sqlite::Transaction> transactionSQLite = nullptr;

public:
  explicit Transaction(const Connection &connection);

  Transaction(Transaction &transaction) = delete;

  void start();

  void commit();

  void save(const std::string &savepoint) const;

  void rollback();

  void rollbackTo(const std::string &savepoint) const;
};
} // namespace db

#endif // CPP_BASE_LIBRARY_TRANSACTION_H
