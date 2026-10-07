#ifndef LINTEL_PREPAREDSTATEMENT_H
#define LINTEL_PREPAREDSTATEMENT_H

#include <string_view>

#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/ParameterBuilder.h"
#include "lintel/core/persistence/Result.h"
#include "lintel/core/persistence/postgresql/PreparedStatement.h"
#include "lintel/core/persistence/sqlite3/PreparedStatement.h"

namespace db {
    /**
     * Unified prepared statement wrapping PostgreSQL or SQLite prepared statements.
     */
    class PreparedStatement {
    private:
        const Connection &m_connection;
        std::unique_ptr<postgresql::PreparedStatement> m_preparedStatement = nullptr;
        std::unique_ptr<sqlite::PreparedStatement> m_preparedStatementSQLite =
                nullptr;

    public:
        /**
         * Counts the number of parameter placeholders in a statement template.
         * @param tempStatement the statement template with '?' placeholders
         * @return the number of parameters
         */
        static constexpr int32_t initNParams(std::string_view tempStatement) {
            return postgresql::PreparedStatement::initNParams(tempStatement);
        }

        /**
         * Converts a statement template with '?' placeholders to the native format.
         * @param tempStatement the statement template
         * @return the converted statement string
         */
        static std::string initStatement(const std::string &tempStatement);

        /**
         * Prepares a statement on the given connection.
         * @param connection the database connection
         * @param statement the SQL statement template
         * @param statementName the name for the prepared statement
         */
        explicit PreparedStatement(const Connection &connection,
                                   const std::string &statement,
                                   const std::string &statementName);

        ~PreparedStatement() = default;

        /**
         * Executes the prepared statement with the given parameters.
         * @param builder the parameter builder containing parameter values
         * @return the query result
         */
        Result execute(const ParameterBuilder &builder);

        /** Closes the prepared statement and frees associated resources. */
        void close();
    };
}  // namespace db

#endif  // LINTEL_PREPAREDSTATEMENT_H
