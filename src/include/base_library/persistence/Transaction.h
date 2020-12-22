#ifndef CPP_BASE_LIBRARY_TRANSACTION_H
#define CPP_BASE_LIBRARY_TRANSACTION_H

#include "Connection.h"
#include "base_library/exceptions/SQLException.h"
#include <string>

namespace db {
class Transaction {
private:
  const Connection &connection;
  bool finished = false;

  void checkState(const Result &result);

public:
  explicit Transaction(const Connection &tempConnection);

  Transaction(Transaction &transaction) = delete;

  void start();

  void commit();

  void save(const std::string &savepoint) const;

  void rollback();

  void rollbackTo(const std::string &savepoint) const;

  ~Transaction();
};
} // namespace db

#endif // CPP_BASE_LIBRARY_TRANSACTION_H
