#ifndef LINTEL_SQLITE_STATEMENT_H
#define LINTEL_SQLITE_STATEMENT_H

#include "Connection.h"

namespace sqlite {
    /**
     * Executes SQL statements on an SQLite connection.
     */
    class Statement {
    private:
        Connection &m_connection;

    public:
        /**
         * Constructs a Statement bound to the given connection.
         * @param connection the SQLite connection
         */
        explicit Statement(Connection &connection);

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
}  // namespace sqlite

#endif  // LINTEL_SQLITE_STATEMENT_H
