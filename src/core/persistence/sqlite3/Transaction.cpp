#include "base_library/core/persistence/sqlite3/Transaction.h"

namespace sqlite {
Transaction::Transaction(const Connection &tempConnection)
    : connection(tempConnection) {
  auto result = connection.execute("BEGIN TRANSACTION");
}

void Transaction::start() {
  if (!finished) {
    return;
  }
  auto result = connection.execute("BEGIN TRANSACTION");
  finished = false;
}

void Transaction::commit() {
  isFinished();
  auto result = connection.execute("COMMIT");
  finished = true;
}

void Transaction::rollback() {
  isFinished();
  auto result = connection.execute("ROLLBACK");
  finished = true;
}
void Transaction::isFinished() const {
  if (finished) {
    throw db::SQLException(
        "SQLite Transaction was finished before with commit or rollback");
  }
}

Transaction::~Transaction() {
  if (finished) {
    return;
  }
  auto result = connection.execute("END");
}

void Transaction::save(const std::string &savepoint) const {
  isFinished();
  auto result = connection.execute("SAVEPOINT " + savepoint);
}
void Transaction::rollbackTo(const std::string &savepoint) const {
  auto result = connection.execute("ROLLBACK TO " + savepoint);
}
}  // namespace sqlite