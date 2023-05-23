#include "base_library/core/persistence/sqlite3/Statement.h"

namespace sqlite {
    Statement::Statement(Connection &connection) : m_connection(connection) {}

    std::shared_ptr<Result> Statement::execute(const std::string &query) {
        return m_connection.execute(query);
    }

    std::shared_ptr<Result> Statement::execute(
            const std::string &query, const std::vector<std::string> &params) {
        return m_connection.executeParameters(query, db::Parameters(params));
    }
}  // namespace sqlite