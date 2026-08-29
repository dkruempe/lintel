#ifndef CPP_BASE_LIBRARY_SQLITE_CURSOR_H
#define CPP_BASE_LIBRARY_SQLITE_CURSOR_H

#include <sqlite3.h>

#include <cstddef>
#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"

namespace sqlite {
    class Connection;

    /**
     * Forward-only cursor over an SQLite query result.
     * Rows are fetched lazily via sqlite3_step() instead of buffering the
     * whole result set. The cursor owns its statement and finalizes it on
     * destruction; the underlying Connection must outlive the cursor.
     *
     * The @p fetchSize parameter controls the client-side prefetch buffer
     * size (how many rows are buffered per fetchNext() call).  SQLite has
     * no server-side cursor concept, so fetchSize only affects client
     * memory usage, not server behaviour.
     */
    class Cursor {
    private:
        sqlite3 *m_db;
        sqlite3_stmt *m_stmt;
        bool m_exhausted;
        std::size_t m_fetchSize;

    public:
        /** Default client-side prefetch buffer size */
        static constexpr std::size_t kDefaultFetchSize = 64;

        /**
         * Prepares and binds the query for streaming execution.
         * @param connection the SQLite connection (must outlive the cursor)
         * @param query the SQL query with '?' placeholders
         * @param params the parameter values to bind
         * @param fetchSize client-side prefetch buffer size (default 64)
         * @throws db::SQLException on prepare or bind failure
         */
        explicit Cursor(const Connection &connection, const std::string &query,
                        const std::vector<std::string> &params,
                        std::size_t fetchSize = kDefaultFetchSize);

        Cursor(Cursor &cursor) = delete;

        /** Destructor, finalizes the statement. */
        ~Cursor();

        /**
         * Fetches up to maxRows further rows from the query.
         * Internally buffers up to fetchSize rows per sqlite3_step() call.
         * @param maxRows maximum number of rows to fetch (must be > 0)
         * @return the fetched rows; fewer rows than requested means the
         *         result set is exhausted
         * @throws db::SQLException on step failure
         */
        [[nodiscard]] std::vector<db::Arguments> fetchNext(std::size_t maxRows);
    };
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_CURSOR_H
