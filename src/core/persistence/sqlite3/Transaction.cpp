#include "base_library/core/persistence/sqlite3/Transaction.h"

namespace sqlite {
Transaction::Transaction(const Connection &tempConnection)
    : m_connection(tempConnection) {
  auto result = m_connection.execute("BEGIN TRANSACTION");
}

void Transaction::start() {
  if (!m_finished) {
    return;
  }
  auto result = m_connection.execute("BEGIN TRANSACTION");
  m_finished = false;
}

void Transaction::commit() {
  isFinished();
  auto result = m_connection.execute("COMMIT");
  m_finished = true;
}

void Transaction::rollback() {
  isFinished();
  auto result = m_connection.execute("ROLLBACK");
  m_finished = true;
}
void Transaction::isFinished() const {
  if (m_finished) {
    throw db::SQLException(
        "SQLite Transaction was finished before with commit or rollback");
  }
}

Transaction::~Transaction() {
  if (m_finished) {
    return;
  }
  auto result = m_connection.execute("END");
}

void Transaction::save(const std::string &savepoint) const {
  isFinished();
  auto result = m_connection.execute("SAVEPOINT " + savepoint);
}
void Transaction::rollbackTo(const std::string &savepoint) const {
  auto result = m_connection.execute("ROLLBACK TO " + savepoint);
}
}  // namespace sqlite