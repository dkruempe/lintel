#include "base_library/core/persistence/postgresql/PreparedStatement.h"

#include <algorithm>

#include "base_library/core/persistence/Parameter.h"

namespace postgresql {
    int32_t PreparedStatement::initNParams(const std::string &tempStatement) {
        return static_cast<int32_t>(
                std::count_if(tempStatement.begin(), tempStatement.end(),
                              [](char temp) { return temp == '?'; }));
    }

    std::string PreparedStatement::initStatement(const std::string &tempStatement) {
        std::string temp;
        int32_t counter = 0;
        for (const char &iter: tempStatement) {
            if (iter == '?') {
                temp += "$" + std::to_string(++counter);
            } else {
                temp += iter;
            }
        }
        return temp;
    }

    PreparedStatement::PreparedStatement(const Connection &connection,
                                         const std::string &statement,
                                         const std::string &statementName)
            : m_connection(connection),
              m_nParams(initNParams(statement)),
              m_statementName(statementName),
              m_statement(initStatement(statement)) {
        std::shared_ptr<Result> result =
                connection.prepareStatement(m_statementName, m_statement, m_nParams);
        if (!result->isState(PGRES_COMMAND_OK)) {
            throw db::SQLException("Prepare of statement '" + statementName +
                                   "' failed: " + connection.getErrorMessage());
        }
    }

    std::shared_ptr<Result> PreparedStatement::execute(
            const std::vector<std::string> &params) {
        if (m_closed) {
            throw db::SQLException("Prepared Statement '" + m_statementName +
                                   "' closed");
        }

        std::shared_ptr<Result> result = m_connection.executePreparedStatement(
                m_statementName, m_statement, m_nParams, db::Parameters(params));
        if (!result->isState(PGRES_TUPLES_OK) && !result->isState(PGRES_COMMAND_OK)) {
            throw db::SQLException("Prepared execution failed: " +
                                   m_connection.getErrorMessage());
        }
        return result;
    }

    void PreparedStatement::close() {
        if (m_closed) {
            return;
        }
        std::shared_ptr<Result> result =
                m_connection.execute("DEALLOCATE " + m_statementName);
        if (!result->isState(PGRES_COMMAND_OK)) {
            throw db::SQLException("Closing of prepared statement failed: " +
                                   m_connection.getErrorMessage());
        }
        m_closed = true;
    }
}  // namespace postgresql