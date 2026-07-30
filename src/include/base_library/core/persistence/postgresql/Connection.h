#ifndef CPP_BASE_LIBRARY_POSTGRES_CONNECTION_H
#define CPP_BASE_LIBRARY_POSTGRES_CONNECTION_H

#include <libpq-fe.h>

#include <memory>
#include <string>
#include <vector>

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/core/persistence/Parameter.h"
#include "base_library/core/persistence/postgresql/Result.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

namespace postgresql {
    class Transaction;

    class Statement;

    class PreparedStatement;

    class Notify;

    /**
     * PostgreSQL database connection wrapping a PGconn handle.
     */
    class Connection {
    private:
        PGconn *m_conn;

        friend class Transaction;

        friend class Statement;

        friend class PreparedStatement;

        friend class Notify;

        /**
         * Executes a plain SQL statement.
         * @param statement the SQL statement
         * @return shared pointer to the result
         */
        std::shared_ptr<Result> execute(
                const std::string &statement) const;

        /**
         * Executes a parameterized SQL statement.
         * @param statement the SQL statement with placeholders
         * @param parameters the parameters to bind
         * @return shared pointer to the result
         */
        [[nodiscard]] std::shared_ptr<Result> executeParameters(
                const std::string &statement, const db::Parameters &parameters) const;

        /**
         * Prepares a named statement for repeated execution.
         * @param statementName the name for the prepared statement
         * @param query the SQL query
         * @param nParams the number of parameters
         * @return shared pointer to the result
         */
        [[nodiscard]] std::shared_ptr<Result> prepareStatement(
                const std::string &statementName, const std::string &query,
                int32_t nParams) const;

        /**
         * Executes a previously prepared statement.
         * @param statementName the name of the prepared statement
         * @param query the SQL query
         * @param nParams the number of parameters
         * @param parameters the parameters to bind
         * @return shared pointer to the result
         */
        [[nodiscard]] std::shared_ptr<Result> executePreparedStatement(
                const std::string &statementName, const std::string &query,
                int32_t nParams, const db::Parameters &parameters) const;

        /** @return the last error message from the PostgreSQL connection */
        [[nodiscard]] std::string getErrorMessage() const;

    public:
        /**
         * Constructs a PostgreSQL connection from a connection string.
         * @param connectionInfo the PostgreSQL connection string
         */
        explicit Connection(const std::string &connectionInfo);

        /**
         * Constructs a PostgreSQL connection from a configuration entry.
         * @param connectionEntry the database connection configuration entry
         */
        explicit Connection(
                const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

        Connection(Connection &connection) = delete;

        /** Destructor, closes the connection. */
        ~Connection();
    };
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRES_CONNECTION_H
