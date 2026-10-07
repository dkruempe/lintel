#include "lintel/core/persistence/postgresql/Cursor.h"

#include "lintel/core/exceptions/SQLException.h"
#include "lintel/core/persistence/postgresql/Connection.h"

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

    /**
     * Sends a query via PQsendQueryParams, waits for the result, parses
     * PGRES_TUPLES_OK into rows and drains everything else.
     * Returns the rows or throws on error.
     */
    std::vector<db::Arguments> sendAndCollect(
            PGconn *conn, const std::string &sql,
            const std::vector<std::string> &params) {
        std::vector<const char *> values;
        values.reserve(params.size());
        for (const auto &p: params) {
            values.push_back(p.c_str());
        }
        const int sent = PQsendQueryParams(
                conn, sql.c_str(),
                static_cast<int>(params.size()), nullptr,
                values.empty() ? nullptr : values.data(),
                nullptr, nullptr, 0);
        if (sent == 0) {
            throw db::SQLException("PostgreSQL cursor query send failed: " +
                                   std::string(PQerrorMessage(conn)));
        }
        std::vector<db::Arguments> rows;
        while (PGresult *res = PQgetResult(conn)) {
            const ExecStatusType status = PQresultStatus(res);
            if (status == PGRES_TUPLES_OK) {
                const int nrows = PQntuples(res);
                for (int i = 0; i < nrows; i++) {
                    // convertRow expects row 0; for multi-row results we
                    // build Arguments manually per row.
                    const int ncols = PQnfields(res);
                    db::Arguments args(db::ConnectionType::PostgreSQL);
                    for (int j = 0; j < ncols; j++) {
                        std::string value;
                        if (!PQgetisnull(res, i, j)) {
                            value = PQgetvalue(res, i, j);
                        }
                        std::string name = PQfname(res, j);
                        db::Argument arg(value, name);
                    args.add(arg);
                    }
                    rows.push_back(std::move(args));
                }
                PQclear(res);
            } else if (status == PGRES_COMMAND_OK) {
                PQclear(res);
            } else {
                std::string error = PQresultErrorMessage(res);
                PQclear(res);
                throw db::SQLException("PostgreSQL cursor query failed: " + error);
            }
        }
        return rows;
    }

    /**
     * Generates a unique cursor name.
     */
    std::string generateCursorName() {
        static std::atomic<std::size_t> counter{0};
        return "cur_" + std::to_string(counter.fetch_add(1, std::memory_order_relaxed));
    }

}  // namespace

namespace postgresql {
    // ── Streaming constructor (status quo) ───────────────────────────
    Cursor::Cursor(const Connection &connection, const std::string &query,
                   const std::vector<std::string> &params)
        : m_conn(connection.m_conn),
          m_active(true),
          m_serverSide(false),
          m_fetchSize(0) {
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

    // ── Server-side cursor constructor ───────────────────────────────
    Cursor::Cursor(const Connection &connection, const std::string &query,
                   const std::vector<std::string> &params,
                   std::size_t fetchSize, const std::string &cursorName,
                   bool noScroll)
        : m_conn(connection.m_conn),
          m_active(true),
          m_serverSide(true),
          m_cursorName(cursorName.empty() ? generateCursorName() : cursorName),
          m_fetchSize(fetchSize) {
        if (fetchSize == 0) {
            throw db::SQLException("PostgreSQL server-side cursor requires fetchSize > 0");
        }

        // 1. DECLARE CURSOR
        std::string scroll = noScroll ? " NO SCROLL" : "";
        std::string declareSql = "DECLARE " + m_cursorName + scroll +
                                 " CURSOR FOR " + query;
        sendAndCollect(m_conn, declareSql, params);
    }

    void Cursor::drain() {
        while (PGresult *res = PQgetResult(m_conn)) {
            PQclear(res);
        }
    }

    void Cursor::close() {
        if (!m_serverSide || !m_active) {
            return;
        }
        std::string closeSql = "CLOSE " + m_cursorName;
        // ignore errors — cursor may already be closed
        PQsendQueryParams(m_conn, closeSql.c_str(), 0, nullptr, nullptr,
                          nullptr, nullptr, 0);
        drain();
        m_active = false;
    }

    Cursor::~Cursor() {
        if (m_active) {
            if (m_serverSide) {
                close();
            } else {
                drain();
            }
            m_active = false;
        }
    }

    // ── Server-side fetch ────────────────────────────────────────────
    std::vector<db::Arguments> Cursor::fetchFromServer(std::size_t maxRows) {
        std::string fetchSql = "FETCH " + std::to_string(maxRows) +
                               " FROM " + m_cursorName;
        return sendAndCollect(m_conn, fetchSql, {});
    }

    // ── Unified fetchNext ────────────────────────────────────────────
    std::vector<db::Arguments> Cursor::fetchNext(std::size_t maxRows) {
        if (m_serverSide) {
            auto rows = fetchFromServer(maxRows);
            // Fewer rows than requested => cursor exhausted (see the
            // @return contract in Cursor.h).  Nothing is closed here on
            // purpose: the destructor issues the CLOSE, so the caller can
            // still inspect its state before the cursor goes away.  A
            // subsequent fetchNext() simply yields an empty batch.
            return rows;
        }

        // Streaming mode (status quo)
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
