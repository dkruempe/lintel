#ifndef CPP_BASE_LIBRARY_POSTGRESQL_STATEMENT_H
#define CPP_BASE_LIBRARY_POSTGRESQL_STATEMENT_H

#include "Connection.h"

namespace postgresql {
    class Statement {
    private:
        const Connection &m_connection;

        static int32_t initNParams(const std::string &tempStatement);

        static std::string initStatement(const std::string &tempStatement);

    public:
        explicit Statement(const Connection &connection);

        std::shared_ptr<Result> execute(const std::string &query);

        std::shared_ptr<Result> execute(const std::string &query,
                                        const std::vector<std::string> &params);
    };
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_STATEMENT_H
