#ifndef CPP_BASE_LIBRARY_SQLITE_TRANSACTION_H
#define CPP_BASE_LIBRARY_SQLITE_TRANSACTION_H

#include "Connection.h"
#include "base_library/core/exceptions/SQLException.h"
#include <string>

namespace sqlite {
class Transaction {
private:
  const Connection &connection;
  bool finished = false;

public:
  explicit Transaction(const Connection &tempConnection);

  Transaction(Transaction &transaction) = delete;

  void start();

  void commit();

  void save(const std::string &savepoint) const;

  void rollback();

  void rollbackTo(const std::string &savepoint) const;

  ~Transaction();
  void isFinished() const;
};
} // namespace sqlite

#endif // CPP_BASE_LIBRARY_SQLITE_TRANSACTION_H
