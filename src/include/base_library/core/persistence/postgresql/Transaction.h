#ifndef CPP_BASE_LIBRARY_POSTGRESQL_TRANSACTION_H
#define CPP_BASE_LIBRARY_POSTGRESQL_TRANSACTION_H

#include <string>

#include "Connection.h"
#include "base_library/core/exceptions/SQLException.h"

namespace postgresql {
    /**
     * Manages a PostgreSQL database transaction with commit, rollback, and savepoint support.
     */
    class Transaction {
    private:
        const Connection &m_connection;
        bool m_finished = false;

        /**
         * Checks the result state and throws if an error occurred.
         * @param result the result to check
         * @throws SQLException if the result indicates an error
         */
        void checkState(const std::shared_ptr<Result> &result);

    public:
        /**
         * Constructs a Transaction bound to the given connection.
         * @param tempConnection the PostgreSQL connection
         */
        explicit Transaction(const Connection &tempConnection);

        Transaction(Transaction &transaction) = delete;

        /** Begins the transaction with a BEGIN statement. */
        void start();

        /** Commits the transaction with a COMMIT statement. */
        void commit();

        /**
         * Creates a savepoint within the transaction.
         * @param savepoint the savepoint name
         */
        void save(const std::string &savepoint) const;

        /** Rolls back the transaction with a ROLLBACK statement. */
        void rollback();

        /**
         * Rolls back to a named savepoint.
         * @param savepoint the savepoint name
         */
        void rollbackTo(const std::string &savepoint) const;

        /** Destructor, rolls back if not finished. */
        ~Transaction();
    };
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_TRANSACTION_H
