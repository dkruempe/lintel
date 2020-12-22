#include "base_library/persistence/database/Transaction.h"

namespace db {
void Transaction::checkState(const Result &result) {
  if (!result.isState(PGRES_COMMAND_OK)) {
    throw SQLException("Connection to database failed: " +
                       connection.getErrorMessage());
  }
}

Transaction::Transaction(const Connection &tempConnection)
    : connection(tempConnection) {
  const Result &result = connection.execute("BEGIN");
  checkState(result);
}

void Transaction::start() {
  if (!finished) {
    return;
  }
  // start transaction again
  const Result &result = connection.execute("BEGIN");
  checkState(result);
  finished = false;
}

void Transaction::commit() {
  if (finished) {
    throw SQLException("Transaction is finished and not available anymore");
  }
  const Result &result = connection.execute("COMMIT");
  checkState(result);
  finished = true;
}

void Transaction::save(const std::string &savepoint) const {
  if (finished) {
    throw SQLException("Transaction is finished and not available anymore");
  }
  const Result &result = connection.execute(
      "SAVEPOINT " + savepoint); // saves current state of transaction
}

void Transaction::rollback() {
  if (finished) {
    throw SQLException("Transaction is finished and not available anymore");
  }
  const Result &result = connection.execute("ROLLBACK");
  checkState(result);
  finished = true;
}

void Transaction::rollbackTo(const std::string &savepoint) const {
  if (finished) {
    throw SQLException("Transaction is finished and not available anymore");
  }
  const Result &result = connection.execute("ROLLBACK TO " + savepoint);
}

Transaction::~Transaction() {
  if (finished) {
    return;
  }
  const Result &result = connection.execute("END");
  checkState(result);
}
} // namespace db