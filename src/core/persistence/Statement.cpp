#include "base_library/core/persistence/Statement.h"

#include "base_library/core/persistence/postgresql/Cursor.h"
#include "base_library/core/persistence/sqlite3/Cursor.h"
#include "base_library/core/persistence/postgresql/Statement.h"

namespace db {
    Statement::Statement(const Connection &connection) : m_connection(connection) {
        switch (connection.m_connectionType) {
            case ConnectionType::SQLite:
                m_statementSQLite =
                        std::make_unique<sqlite::Statement>(*connection.m_connSQLite);
                break;
            case ConnectionType::PostgreSQL:
                m_statement = std::make_unique<postgresql::Statement>(*connection.m_conn);
                break;
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
    }

    Result Statement::execute(const std::string &query) {
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                return Result(m_statementSQLite->execute(query));
                break;
            case ConnectionType::PostgreSQL:
                return Result(m_statement->execute(query));
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
        return {};
    }

    Result Statement::execute(const std::string &query,
                              const ParameterBuilder &builder) {
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                return Result(m_statementSQLite->execute(query, builder.build()));
                break;
            case ConnectionType::PostgreSQL:
                return Result(m_statement->execute(query, builder.build()));
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
        return {};
    }

    Cursor Statement::executeCursor(const std::string &query) {
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                return Cursor(nullptr,
                              std::make_shared<sqlite::Cursor>(
                                      *m_connection.m_connSQLite, query,
                                      std::vector<std::string>{}));
            case ConnectionType::PostgreSQL:
                return Cursor(std::make_shared<postgresql::Cursor>(
                                      *m_connection.m_conn,
                                      postgresql::Statement::initStatement(query),
                                      std::vector<std::string>{}),
                              nullptr);
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
        return {};
    }

    Cursor Statement::executeCursor(const std::string &query,
                                    const ParameterBuilder &builder) {
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                return Cursor(nullptr,
                              std::make_shared<sqlite::Cursor>(
                                      *m_connection.m_connSQLite, query,
                                      builder.build()));
            case ConnectionType::PostgreSQL:
                return Cursor(std::make_shared<postgresql::Cursor>(
                                      *m_connection.m_conn,
                                      postgresql::Statement::initStatement(query),
                                      builder.build()),
                              nullptr);
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
        return {};
    }
}  // namespace db