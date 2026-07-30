#ifndef CPP_BASE_LIBRARY_STATEMENT_H
#define CPP_BASE_LIBRARY_STATEMENT_H

#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/ParameterBuilder.h"
#include "base_library/core/persistence/Result.h"
#include "base_library/core/persistence/postgresql/Statement.h"
#include "base_library/core/persistence/sqlite3/Statement.h"

namespace db {
    /**
     * Unified statement execution wrapping PostgreSQL or SQLite statements.
     */
    class Statement {
    private:
        std::unique_ptr<postgresql::Statement> m_statement = nullptr;
        std::unique_ptr<sqlite::Statement> m_statementSQLite = nullptr;
        const Connection &m_connection;

    public:
        /**
         * Constructs a Statement bound to the given connection.
         * @param connection the database connection
         */
        explicit Statement(const Connection &connection);

        /**
         * Executes a plain SQL query.
         * @param query the SQL query string
         * @return the query result
         */
        Result execute(const std::string &query);

        /**
         * Executes a parameterized SQL query.
         * @param query the SQL query string with placeholders
         * @param builder the parameter builder containing parameter values
         * @return the query result
         */
        Result execute(const std::string &query, const db::ParameterBuilder &builder);
    };
}  // namespace db

#endif  // CPP_BASE_LIBRARY_STATEMENT_H
