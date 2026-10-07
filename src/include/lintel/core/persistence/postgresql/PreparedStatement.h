#ifndef LINTEL_POSTGRESQL_PREPAREDSTATEMENT_H
#define LINTEL_POSTGRESQL_PREPAREDSTATEMENT_H

#include <string_view>

#include "Connection.h"

namespace postgresql {
/**
 * format of prepared statement
 * example:
 * insert into table (..,..) values(?,?);
 *
 * => ? will be replace automatically with $1 and $2
 * => algorithm will automatically count the number of parameters
 */
    class PreparedStatement {
    private:
        bool m_closed = false;
        const Connection &m_connection;
        const int32_t m_nParams;
        const std::string m_statementName;
        const std::string m_statement;

    public:
        /**
         * Counts the number of '?' placeholders in a statement template.
         * @param tempStatement the statement template
         * @return the number of parameters
         */
        static constexpr int32_t initNParams(std::string_view tempStatement) {
            int32_t count = 0;
            for (const char c : tempStatement) {
                if (c == '?') {
                    ++count;
                }
            }
            return count;
        }

        /**
         * Converts '?' placeholders to PostgreSQL's $1, $2, ... format.
         * @param tempStatement the statement template with '?' placeholders
         * @return the converted statement
         */
        static std::string initStatement(const std::string &tempStatement);

        /**
         * Prepares a statement on the given connection.
         * @param connection the PostgreSQL connection
         * @param statement the SQL statement template with '?' placeholders
         * @param statementName the name for the prepared statement
         */
        PreparedStatement(const Connection &connection, const std::string &statement,
                          const std::string &statementName);

        ~PreparedStatement() = default;

        /**
         * Executes the prepared statement with the given parameters.
         * @param params the parameter values as strings
         * @return shared pointer to the result
         */
        std::shared_ptr<Result> execute(const std::vector<std::string> &params);

        /** Closes the prepared statement. */
        void close();
    };
}  // namespace postgresql
#endif  // LINTEL_POSTGRESQL_PREPAREDSTATEMENT_H
