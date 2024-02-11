
#include "base_library/core/persistence/postgresql/Result.h"

namespace postgresql {
    Result::Result(PGresult *res) : m_res(res) {
        int cols = PQntuples(m_res);
        int row = PQnfields(m_res);
        for (int i = 0; i < cols; i++) {
            db::Arguments arguments(db::ConnectionType::PostgreSQL);
            for (int j = 0; j < row; j++) {
                std::string value = PQgetvalue(m_res, static_cast<int>(i), j);
                std::string name = PQfname(m_res, j);
                db::Argument argument(value, name);
                arguments.add(argument);
            }
            m_arguments.push_back(arguments);
        }
    }

    Result::~Result() {
        if (m_res != nullptr) {
            PQclear(m_res);
            m_res = nullptr;
        }
    }

    [[nodiscard]] bool Result::isState(ExecStatusType status) const {
        return PQresultStatus(m_res) == status;
    }

    [[nodiscard]] std::string Result::getValue(int row, int attribute) const {
        return PQgetvalue(m_res, row, attribute);
    }

    [[nodiscard]] int Result::getNumOfAttributes() const {
        return PQnfields(m_res);
    }

    [[nodiscard]] int Result::getSize() const { return PQntuples(m_res); }

    db::Arguments &Result::of(std::size_t pos) { return m_arguments.at(pos); }
}  // namespace postgresql
