#ifndef CPP_BASE_LIBRARY_SQLITE_CURSOR_H
#define CPP_BASE_LIBRARY_SQLITE_CURSOR_H

#include <sqlite3.h>

#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"

namespace sqlite {
    class Connection;

    /**
     * Forward-only streaming cursor over an SQLite query result.
     * Rows are fetched lazily via sqlite3_step() instead of buffering the
     * whole result set. The cursor owns its statement and finalizes it on
     * destruction; the underlying Connection must outlive the cursor.
     */
    class Cursor {
    private:
        sqlite3 *m_db;
        sqlite3_stmt *m_stmt;
        bool m_exhausted;

    public:
        /**
         * Prepares and binds the query for streaming execution.
         * @param connection the SQLite connection (must outlive the cursor)
         * @param query the SQL query with '?' placeholders
         * @param params the parameter values to bind
         * @throws db::SQLException on prepare or bind failure
         */
        explicit Cursor(const Connection &connection, const std::string &query,
                        const std::vector<std::string> &params);

        Cursor(Cursor &cursor) = delete;

        /** Destructor, finalizes the statement. */
        ~Cursor();

        /**
         * Fetches up to maxRows further rows from the query.
         * @param maxRows maximum number of rows to fetch (must be > 0)
         * @return the fetched rows; fewer rows than requested means the
         *         result set is exhausted
         * @throws db::SQLException on step failure
         */
        [[nodiscard]] std::vector<db::Arguments> fetchNext(std::size_t maxRows);
    };
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_CURSOR_H
