#include "base_library/core/persistence/Statement.h"

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
        Cursor cursor;
        cursor.m_query = query;
        cursor.m_needsInit = true;
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                cursor.m_sqliteConn = m_connection.m_connSQLite;
                break;
            case ConnectionType::PostgreSQL:
                cursor.m_pgConn = m_connection.m_conn;
                break;
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
        return cursor;
    }

    Cursor Statement::executeCursor(const std::string &query,
                                    const ParameterBuilder &builder) {
        Cursor cursor;
        cursor.m_query = query;
        cursor.m_params = builder.build();
        cursor.m_needsInit = true;
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                cursor.m_sqliteConn = m_connection.m_connSQLite;
                break;
            case ConnectionType::PostgreSQL:
                cursor.m_pgConn = m_connection.m_conn;
                break;
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
        return cursor;
    }
}  // namespace db