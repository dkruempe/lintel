#ifndef CPP_BASE_LIBRARY_CONNECTION_H
#define CPP_BASE_LIBRARY_CONNECTION_H

#include <memory>

#include "base_library/core/persistence/postgresql/Connection.h"
#include "base_library/core/persistence/sqlite3/Connection.h"
#include "base_library/features/base/configuration/DatabaseConnectionEntry.h"

namespace db {

    class Transaction;

    class Statement;

    class PreparedStatement;

    class Notify;

    class Connection {
    private:
        std::shared_ptr<postgresql::Connection> m_conn = nullptr;
        std::shared_ptr<sqlite::Connection> m_connSQLite = nullptr;

        ConnectionType m_connectionType;

        friend class Transaction;

        friend class Statement;

        friend class PreparedStatement;

        friend class Notify;

    public:
        explicit Connection(ConnectionType connectionType,
                            const std::string &connectionInfo);

        explicit Connection(
                const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry);

        explicit Connection(Connection &connection) = delete;
    };
}  // namespace db

#endif  // CPP_BASE_LIBRARY_CONNECTION_H
