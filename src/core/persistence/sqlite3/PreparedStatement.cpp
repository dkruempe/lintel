#include "base_library/core/persistence/sqlite3/PreparedStatement.h"

#include "base_library/core/exceptions/SQLException.h"

namespace sqlite {
    PreparedStatement::PreparedStatement(Connection &connection,
                                         const std::string &statement,
                                         const std::string &statementName)
            : m_connection(connection),
              m_statement(statement),
              m_statementName(statementName) {
        connection.prepareStatement(statementName, statement);
    }

    std::shared_ptr<Result> PreparedStatement::execute(
            const std::vector<std::string> &params) {
        if (m_closed) {
            throw db::SQLException(
                    "SQLite: prepared statement is closed => abort execute");
        }
        db::Parameters parameters(params);
        return m_connection.executePreparedStatement(m_statementName, parameters);
    }

    void PreparedStatement::close() {
        m_closed = true;
        m_connection.finalizePreparedStatement(m_statementName);
    }

    PreparedStatement::~PreparedStatement() {
        m_connection.finalizePreparedStatement(m_statementName);
    }
}  // namespace sqlite