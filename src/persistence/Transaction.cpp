#include "base_library/persistence/Transaction.h"
#include "base_library/persistence/postgresql/Transaction.h"

namespace db {
Transaction::Transaction(const Connection &connection)
    : connection(connection) {
  switch (this->connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    transaction = std::make_unique<postgresql::Transaction>(*this->connection.conn);
    break;
  }
}

void Transaction::start() {
  switch (connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    transaction->start();
    break;
  }
}

void Transaction::commit() {
  switch (connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    transaction->commit();
    break;
  }
}

void Transaction::rollbackTo(const std::string &savepoint) const {
  switch (connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    transaction->rollbackTo(savepoint);
    break;
  }
}

void Transaction::rollback() {
  switch (connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    transaction->rollback();
    break;
  }
}

void Transaction::save(const std::string &savepoint) const {
  switch (connection.connectionType) {
  case SQLite:
    break;
  case PostgreSQL:
    transaction->save(savepoint);
    break;
  }
}
} // namespace db