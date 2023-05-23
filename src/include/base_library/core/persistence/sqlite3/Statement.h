#ifndef CPP_BASE_LIBRARY_SQLITE_STATEMENT_H
#define CPP_BASE_LIBRARY_SQLITE_STATEMENT_H

#include "Connection.h"

namespace sqlite {
    class Statement {
    private:
        Connection &m_connection;

    public:
        explicit Statement(Connection &connection);

        std::shared_ptr<Result> execute(const std::string &query);

        std::shared_ptr<Result> execute(const std::string &query,
                                        const std::vector<std::string> &params);
    };
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_STATEMENT_H
