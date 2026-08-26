#ifndef CPP_BASE_LIBRARY_POSTGRESQL_STATEMENT_H
#define CPP_BASE_LIBRARY_POSTGRESQL_STATEMENT_H

#include "Connection.h"

namespace postgresql {
    /**
     * Executes SQL statements on a PostgreSQL connection.
     */
    class Statement {
    private:
        const Connection &m_connection;

    public:
        /**
         * Counts the number of '?' placeholders in a statement template.
         * @param tempStatement the statement template
         * @return the number of parameters
         */
        static int32_t initNParams(const std::string &tempStatement);

        /**
         * Converts '?' placeholders to PostgreSQL's $1, $2, ... format.
         * @param tempStatement the statement template
         * @return the converted statement
         */
        static std::string initStatement(const std::string &tempStatement);

        /**
         * Constructs a Statement bound to the given connection.
         * @param connection the PostgreSQL connection
         */
        explicit Statement(const Connection &connection);

        /**
         * Executes a plain SQL query.
         * @param query the SQL query string
         * @return shared pointer to the result
         */
        std::shared_ptr<Result> execute(const std::string &query);

        /**
         * Executes a parameterized SQL query.
         * @param query the SQL query with placeholders
         * @param params the parameter values as strings
         * @return shared pointer to the result
         */
        std::shared_ptr<Result> execute(const std::string &query,
                                        const std::vector<std::string> &params);
    };
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_STATEMENT_H
