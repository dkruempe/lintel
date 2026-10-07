#ifndef LINTEL_TRANSACTION_H
#define LINTEL_TRANSACTION_H

#include "Connection.h"
#include "lintel/core/persistence/postgresql/Transaction.h"
#include "lintel/core/persistence/sqlite3/Transaction.h"

namespace db {
    /**
     * Unified database transaction wrapping PostgreSQL or SQLite transactions.
     */
    class Transaction {
    private:
        const Connection &m_connection;
        std::unique_ptr<postgresql::Transaction> m_transaction = nullptr;
        std::unique_ptr<sqlite::Transaction> m_transactionSQLite = nullptr;

    public:
        /**
         * Constructs a Transaction bound to the given connection.
         * @param connection the database connection
         */
        explicit Transaction(const Connection &connection);

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

        /** Rolls back the entire transaction. */
        void rollback();

        /**
         * Rolls back to a named savepoint.
         * @param savepoint the savepoint name
         */
        void rollbackTo(const std::string &savepoint) const;
    };
}  // namespace db

#endif  // LINTEL_TRANSACTION_H
