#include "base_library/core/persistence/PreparedStatement.h"

#include "base_library/core/persistence/Result.h"
#include "base_library/core/persistence/postgresql/PreparedStatement.h"

namespace db {
    PreparedStatement::PreparedStatement(const Connection &connection,
                                         const std::string &statement,
                                         const std::string &statementName)
            : m_connection(connection) {
        switch (connection.m_connectionType) {
            case ConnectionType::SQLite:
                m_preparedStatementSQLite = std::make_unique<sqlite::PreparedStatement>(
                        *connection.m_connSQLite, statement, statementName);
                break;
            case ConnectionType::PostgreSQL:
                m_preparedStatement = std::make_unique<postgresql::PreparedStatement>(
                        *connection.m_conn, statement, statementName);
                break;
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
    }

    int32_t PreparedStatement::initNParams(const std::string &tempStatement) {
        return postgresql::PreparedStatement::initNParams(tempStatement);
    }

    std::string PreparedStatement::initStatement(const std::string &tempStatement) {
        return postgresql::PreparedStatement::initStatement(tempStatement);
    }

    void PreparedStatement::close() {
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                m_preparedStatementSQLite->close();
                break;
            case ConnectionType::PostgreSQL:
                m_preparedStatement->close();
                break;
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
    }

    Result PreparedStatement::execute(const ParameterBuilder &builder) {
        switch (m_connection.m_connectionType) {
            case ConnectionType::SQLite:
                m_preparedStatementSQLite->execute(builder.build());
                break;
            case ConnectionType::PostgreSQL:
                m_preparedStatement->execute(builder.build());
                break;
            case ConnectionType::UNDEFINED:
                throw db::SQLException("Undefined Database Type");
        }
        return {};
    }
}  // namespace db