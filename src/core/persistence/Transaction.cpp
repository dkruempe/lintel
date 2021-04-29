#include "base_library/core/persistence/Transaction.h"

#include "base_library/core/persistence/postgresql/Transaction.h"

namespace db {
Transaction::Transaction(const Connection &connection)
    : m_connection(connection) {
  switch (this->m_connection.m_connectionType) {
    case ConnectionType::SQLite:
      m_transactionSQLite =
          std::make_unique<sqlite::Transaction>(*m_connection.m_connSQLite);
      break;
    case ConnectionType::PostgreSQL:
      m_transaction =
          std::make_unique<postgresql::Transaction>(*m_connection.m_conn);
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::start() {
  switch (m_connection.m_connectionType) {
    case ConnectionType::SQLite:
      m_transactionSQLite->start();
      break;
    case ConnectionType::PostgreSQL:
      m_transaction->start();
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::commit() {
  switch (m_connection.m_connectionType) {
    case ConnectionType::SQLite:
      m_transactionSQLite->commit();
      break;
    case ConnectionType::PostgreSQL:
      m_transaction->commit();
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::rollbackTo(const std::string &savepoint) const {
  switch (m_connection.m_connectionType) {
    case ConnectionType::SQLite:
      m_transactionSQLite->rollbackTo(savepoint);
      break;
    case ConnectionType::PostgreSQL:
      m_transaction->rollbackTo(savepoint);
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::rollback() {
  switch (m_connection.m_connectionType) {
    case ConnectionType::SQLite:
      m_transactionSQLite->rollback();
      break;
    case ConnectionType::PostgreSQL:
      m_transaction->rollback();
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}

void Transaction::save(const std::string &savepoint) const {
  switch (m_connection.m_connectionType) {
    case ConnectionType::SQLite:
      m_transactionSQLite->save(savepoint);
      break;
    case ConnectionType::PostgreSQL:
      m_transaction->save(savepoint);
      break;
    case ConnectionType::UNDEFINED:
      throw db::SQLException("Undefined Database Type");
  }
}
}  // namespace db