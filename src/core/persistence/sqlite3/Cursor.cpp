#include "base_library/core/persistence/sqlite3/Cursor.h"

#include <algorithm>

#include "base_library/core/exceptions/SQLException.h"
#include "base_library/core/persistence/sqlite3/Connection.h"

namespace {
    db::Arguments convertRow(sqlite3_stmt *stmt) {
        const int count = sqlite3_column_count(stmt);
        db::Arguments arguments(db::ConnectionType::SQLite);
        for (int i = 0; i < count; i++) {
            std::string value;
            if (const unsigned char *text = sqlite3_column_text(stmt, i);
                text != nullptr) {
                std::basic_string<unsigned char> temp = text;
                value = std::string(temp.begin(), temp.end());
            }
            const char *name = sqlite3_column_name(stmt, i);
            if (name == nullptr) {
                continue;
            }
            db::Argument argument(value, name);
            arguments.add(argument);
        }
        return arguments;
    }
}  // namespace

namespace sqlite {
    Cursor::Cursor(const Connection &connection, const std::string &query,
                   const std::vector<std::string> &params,
                   std::size_t fetchSize)
        : m_db(connection.m_db), m_stmt(nullptr), m_exhausted(false),
          m_fetchSize(fetchSize > 0 ? fetchSize : Cursor::kDefaultFetchSize) {
        const int rc = sqlite3_prepare_v2(m_db, query.c_str(),
                                          static_cast<int>(query.size() + 1),
                                          &m_stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw db::SQLException("SQLite cursor prepare exception: " +
                                   std::string(sqlite3_errmsg(m_db)));
        }
        int counter = 0;
        for (const auto &param: params) {
            const int bindRc = sqlite3_bind_text(m_stmt, ++counter, param.c_str(),
                                                 static_cast<int>(param.size()),
                                                 SQLITE_TRANSIENT);
            if (bindRc != SQLITE_OK) {
                sqlite3_finalize(m_stmt);
                m_stmt = nullptr;
                throw db::SQLException("SQLite cursor bind exception: " +
                                       std::string(sqlite3_errmsg(m_db)));
            }
        }
    }

    Cursor::~Cursor() {
        if (m_stmt != nullptr) {
            sqlite3_finalize(m_stmt);
            m_stmt = nullptr;
        }
    }

    std::vector<db::Arguments> Cursor::fetchNext(std::size_t maxRows) {
        const std::size_t limit = std::min(maxRows, m_fetchSize);
        std::vector<db::Arguments> rows;
        rows.reserve(limit);
        while (rows.size() < limit && !m_exhausted) {
            const int step = sqlite3_step(m_stmt);
            switch (step) {
                case SQLITE_ROW:
                    rows.push_back(convertRow(m_stmt));
                    break;
                case SQLITE_DONE:
                    m_exhausted = true;
                    break;
                default:
                    throw db::SQLException("SQLite cursor step exception: " +
                                           std::string(sqlite3_errmsg(m_db)));
            }
        }
        return rows;
    }
}  // namespace sqlite
