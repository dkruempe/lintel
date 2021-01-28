#ifndef CPP_BASE_LIBRARY_TRANSACTION_H
#define CPP_BASE_LIBRARY_TRANSACTION_H

#include "base_library/persistence/Connection.h"
#include "base_library/persistence/postgresql/Transaction.h"

namespace db {
class Transaction {
private:
  const Connection &connection;
  std::unique_ptr<postgresql::Transaction> transaction = nullptr;
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
