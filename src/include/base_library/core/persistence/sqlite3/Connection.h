#ifndef CPP_BASE_LIBRARY_SQLITE_CONNECTION_H
#define CPP_BASE_LIBRARY_SQLITE_CONNECTION_H

#include <sqlite3.h>

#include <map>
#include <memory>
#include <string>

#include "base_library/core/persistence/Parameter.h"
#include "base_library/core/persistence/sqlite3/Result.h"
#include "base_library/core/configuration/DatabaseConnectionEntry.h"

namespace sqlite {
    class Transaction;

    class Statement;

    class PreparedStatement;

    class Notify;

    /**
     * SQLite database connection wrapping a sqlite3 handle.
     */
    class Connection {
    private:
        sqlite3 *m_db;

        friend class Transaction;

        friend class Statement;

        friend class PreparedStatement;

        friend class Notify;

        std::map<std::string, sqlite3_stmt *> m_preparedStatements;

    private:
        /**
         * Executes a plain SQL statement.
         * @param statement the SQL statement
         * @return shared pointer to the result
         */
        [[nodiscard]] std::shared_ptr<Result> execute(
                const std::string &statement) const;

        /**
         * Executes a parameterized SQL statement.
         * @param statement the SQL statement with placeholders
         * @param parameters the parameters to bind
         * @return shared pointer to the result
         */
        [[nodiscard]] std::shared_ptr<Result> executeParameters(
                const std::string &statement, const db::Parameters &parameters);

        /**
         * Prepares a named statement for repeated execution.
         * @param queryName the name for the prepared statement
         * @param query the SQL query
         * @return shared pointer to the result
         */
        std::shared_ptr<Result> prepareStatement(const std::string &queryName,
                                                 const std::string &query);

        /**
         * Executes a previously prepared statement.
         * @param queryName the name of the prepared statement
         * @param parameters the parameters to bind
         * @return shared pointer to the result
         */
        [[nodiscard]] std::shared_ptr<Result> executePreparedStatement(
                const std::string &queryName, const db::Parameters &parameters);

        /**
         * Finalizes (closes) a prepared statement by name.
         * @param queryName the name of the prepared statement
         */
        void finalizePreparedStatement(const std::string &queryName);

        /**
         * Static callback for sqlite3_exec results.
         * @param funcPtr user data pointer
         * @param argc number of columns
         * @param argv column values
         * @param column column names
         * @return 0 on success
         */
        static int callBack(void *funcPtr, int argc, char **argv, char **column);

        /** @return the last error message from SQLite */
        [[nodiscard]] std::string getErrorMessage() const;

    public:
        /**
         * Constructs an SQLite connection from a file path.
         * @param connectionInfo the path to the SQLite database file
         */
        explicit Connection(const std::string &connectionInfo);

        /**
         * Constructs an SQLite connection from a configuration entry.
         * @param connectionEntry the database connection configuration entry
         */
        explicit Connection(
                const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

        Connection(Connection &connection) = delete;

        /** Destructor, closes the database. */
        ~Connection();
    };
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_CONNECTION_H
