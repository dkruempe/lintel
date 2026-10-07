#ifndef LINTEL_SQLITE_TRANSACTION_H
#define LINTEL_SQLITE_TRANSACTION_H

#include <string>

#include "Connection.h"
#include "lintel/core/exceptions/SQLException.h"

namespace sqlite {
    /**
     * Manages an SQLite database transaction with commit, rollback, and savepoint support.
     */
    class Transaction {
    private:
        const Connection &m_connection;
        bool m_finished = false;

    public:
        /**
         * Constructs a Transaction bound to the given connection.
         * @param tempConnection the SQLite connection
         */
        explicit Transaction(const Connection &tempConnection);

        Transaction(Transaction &transaction) = delete;

        /** Begins the transaction. */
        void start();

        /** Commits the transaction. */
        void commit();

        /**
         * Creates a savepoint within the transaction.
         * @param savepoint the savepoint name
         */
        void save(const std::string &savepoint) const;

        /** Rolls back the transaction. */
        void rollback();

        /**
         * Rolls back to a named savepoint.
         * @param savepoint the savepoint name
         */
        void rollbackTo(const std::string &savepoint) const;

        /** Destructor. */
        ~Transaction();

        /** @throw db::SQLException if the transaction was already committed or rolled back */
        void isFinished() const;
    };
}  // namespace sqlite

#endif  // LINTEL_SQLITE_TRANSACTION_H
