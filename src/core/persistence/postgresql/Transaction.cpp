#include "base_library/core/persistence/postgresql/Transaction.h"

namespace postgresql {
    void Transaction::checkState(const std::shared_ptr<Result> &result) {
        if (!result->isState(PGRES_COMMAND_OK)) {
            throw db::SQLException("Connection to database failed: " +
                                   m_connection.getErrorMessage());
        }
    }

    Transaction::Transaction(const Connection &tempConnection)
            : m_connection(tempConnection) {
        std::shared_ptr<Result> result = m_connection.execute("BEGIN");
        checkState(result);
    }

    void Transaction::start() {
        if (!m_finished) {
            return;
        }
        // start transaction again
        std::shared_ptr<Result> result = m_connection.execute("BEGIN");
        checkState(result);
        m_finished = false;
    }

    void Transaction::commit() {
        if (m_finished) {
            throw db::SQLException("Transaction is finished and not available anymore");
        }
        std::shared_ptr<Result> result = m_connection.execute("COMMIT");
        checkState(result);
        m_finished = true;
    }

    void Transaction::save(const std::string &savepoint) const {
        if (m_finished) {
            throw db::SQLException("Transaction is finished and not available anymore");
        }
        std::shared_ptr<Result> result = m_connection.execute(
                "SAVEPOINT " + savepoint);  // saves current state of transaction
    }

    void Transaction::rollback() {
        if (m_finished) {
            throw db::SQLException("Transaction is finished and not available anymore");
        }
        std::shared_ptr<Result> result = m_connection.execute("ROLLBACK");
        checkState(result);
        m_finished = true;
    }

    void Transaction::rollbackTo(const std::string &savepoint) const {
        if (m_finished) {
            throw db::SQLException("Transaction is finished and not available anymore");
        }
        std::shared_ptr<Result> result =
                m_connection.execute("ROLLBACK TO " + savepoint);
    }

    Transaction::~Transaction() {
        if (m_finished) {
            return;
        }
        std::shared_ptr<Result> result = m_connection.execute("END");
        checkState(result);
    }
}  // namespace postgresql
