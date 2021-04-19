#include "base_library/core/persistence/postgresql/Transaction.h"

namespace postgresql {
void Transaction::checkState(const std::shared_ptr<Result> &result) {
  if (!result->isState(PGRES_COMMAND_OK)) {
    throw db::SQLException("Connection to database failed: " +
                           connection.getErrorMessage());
  }
}

Transaction::Transaction(const Connection &tempConnection)
    : connection(tempConnection) {
  std::shared_ptr<Result> result = connection.execute("BEGIN");
  checkState(result);
}

void Transaction::start() {
  if (!finished) {
    return;
  }
  // start transaction again
  std::shared_ptr<Result> result = connection.execute("BEGIN");
  checkState(result);
  finished = false;
}

void Transaction::commit() {
  if (finished) {
    throw db::SQLException("Transaction is finished and not available anymore");
  }
  std::shared_ptr<Result> result = connection.execute("COMMIT");
  checkState(result);
  finished = true;
}

void Transaction::save(const std::string &savepoint) const {
  if (finished) {
    throw db::SQLException("Transaction is finished and not available anymore");
  }
  std::shared_ptr<Result> result = connection.execute(
      "SAVEPOINT " + savepoint);  // saves current state of transaction
}

void Transaction::rollback() {
  if (finished) {
    throw db::SQLException("Transaction is finished and not available anymore");
  }
  std::shared_ptr<Result> result = connection.execute("ROLLBACK");
  checkState(result);
  finished = true;
}

void Transaction::rollbackTo(const std::string &savepoint) const {
  if (finished) {
    throw db::SQLException("Transaction is finished and not available anymore");
  }
  std::shared_ptr<Result> result =
      connection.execute("ROLLBACK TO " + savepoint);
}

Transaction::~Transaction() {
  if (finished) {
    return;
  }
  std::shared_ptr<Result> result = connection.execute("END");
  checkState(result);
}
}  // namespace postgresql
