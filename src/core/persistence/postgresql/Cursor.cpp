#include "base_library/core/persistence/postgresql/Cursor.h"

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/core/persistence/postgresql/Connection.h"

namespace {
    db::Arguments convertRow(const PGresult *res) {
        const int count = PQnfields(res);
        db::Arguments arguments(db::ConnectionType::PostgreSQL);
        for (int j = 0; j < count; j++) {
            std::string value;
            if (!PQgetisnull(res, 0, j)) {
                value = PQgetvalue(res, 0, j);
            }
            std::string name = PQfname(res, j);
            db::Argument argument(value, name);
            arguments.add(argument);
        }
        return arguments;
    }
}  // namespace

namespace postgresql {
    Cursor::Cursor(const Connection &connection, const std::string &query,
                   const std::vector<std::string> &params)
        : m_conn(connection.m_conn), m_active(true) {
        std::vector<const char *> values;
        values.reserve(params.size());
        for (const auto &param: params) {
            values.push_back(param.c_str());
        }
        const int sent = PQsendQueryParams(m_conn, query.c_str(),
                                           static_cast<int>(params.size()),
                                           nullptr,
                                           values.empty() ? nullptr : values.data(),
                                           nullptr, nullptr, 0);
        if (sent == 0) {
            m_active = false;
            throw db::SQLException("PostgreSQL cursor send failed: " +
                                   std::string(PQerrorMessage(m_conn)));
        }
        if (PQsetSingleRowMode(m_conn) == 0) {
            drain();
            m_active = false;
            throw db::SQLException("PostgreSQL cursor single-row mode failed: " +
                                   std::string(PQerrorMessage(m_conn)));
        }
    }

    void Cursor::drain() {
        while (PGresult *res = PQgetResult(m_conn)) {
            PQclear(res);
        }
    }

    Cursor::~Cursor() {
        if (m_active) {
            drain();
            m_active = false;
        }
    }

    std::vector<db::Arguments> Cursor::fetchNext(std::size_t maxRows) {
        std::vector<db::Arguments> rows;
        rows.reserve(maxRows);
        while (rows.size() < maxRows && m_active) {
            PGresult *res = PQgetResult(m_conn);
            if (res == nullptr) {
                m_active = false;
                break;
            }
            const ExecStatusType status = PQresultStatus(res);
            if (status == PGRES_SINGLE_TUPLE) {
                rows.push_back(convertRow(res));
                PQclear(res);
            } else if (status == PGRES_TUPLES_OK) {
                PQclear(res);
            } else {
                std::string error = PQresultErrorMessage(res);
                PQclear(res);
                drain();
                m_active = false;
                throw db::SQLException("PostgreSQL cursor fetch failed: " + error);
            }
        }
        return rows;
    }
}  // namespace postgresql
