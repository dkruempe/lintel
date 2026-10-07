#ifndef LINTEL_STATEMENT_H
#define LINTEL_STATEMENT_H

#include "lintel/core/persistence/Connection.h"
#include "lintel/core/persistence/Cursor.h"
#include "lintel/core/persistence/ParameterBuilder.h"
#include "lintel/core/persistence/Result.h"
#include "lintel/core/persistence/postgresql/Statement.h"
#include "lintel/core/persistence/sqlite3/Statement.h"

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

        /**
         * Executes a plain SQL query as a streaming cursor.
         * @param query the SQL query string
         * @return the cursor; the connection must outlive it
         */
        Cursor executeCursor(const std::string &query);

        /**
         * Executes a parameterized SQL query as a streaming cursor.
         * @param query the SQL query string with placeholders
         * @param builder the parameter builder containing parameter values
         * @return the cursor; the connection must outlive it
         */
        Cursor executeCursor(const std::string &query,
                             const db::ParameterBuilder &builder);
    };
}  // namespace db

#endif  // LINTEL_STATEMENT_H
