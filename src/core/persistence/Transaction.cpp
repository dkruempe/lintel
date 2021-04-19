#include "base_library/core/persistence/Transaction.h"

#include "base_library/core/persistence/postgresql/Transaction.h"

namespace db {
Transaction::Transaction(const Connection &connection)
    : connection(connection) {
  switch (this->connection.connectionType) {
    case ConnectionType::SQLite:
      transactionSQLite =
          std::make_unique<sqlite::Transaction>(*this->connection.connSQLite);
      break;
    case ConnectionType::PostgreSQL:
      transaction =
          std::make_unique<postgresql::Transaction>(*this->connection.conn);
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::start() {
  switch (connection.connectionType) {
    case ConnectionType::SQLite:
      transactionSQLite->start();
      break;
    case ConnectionType::PostgreSQL:
      transaction->start();
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::commit() {
  switch (connection.connectionType) {
    case ConnectionType::SQLite:
      transactionSQLite->commit();
      break;
    case ConnectionType::PostgreSQL:
      transaction->commit();
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::rollbackTo(const std::string &savepoint) const {
  switch (connection.connectionType) {
    case ConnectionType::SQLite:
      transactionSQLite->rollbackTo(savepoint);
      break;
    case ConnectionType::PostgreSQL:
      transaction->rollbackTo(savepoint);
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::rollback() {
  switch (connection.connectionType) {
    case ConnectionType::SQLite:
      transactionSQLite->rollback();
      break;
    case ConnectionType::PostgreSQL:
      transaction->rollback();
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::save(const std::string &savepoint) const {
  switch (connection.connectionType) {
    case ConnectionType::SQLite:
      transactionSQLite->save(savepoint);
      break;
    case ConnectionType::PostgreSQL:
      transaction->save(savepoint);
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}
}  // namespace db