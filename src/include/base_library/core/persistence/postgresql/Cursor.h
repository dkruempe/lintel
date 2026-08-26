#ifndef CPP_BASE_LIBRARY_POSTGRESQL_CURSOR_H
#define CPP_BASE_LIBRARY_POSTGRESQL_CURSOR_H

#include <libpq-fe.h>

#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"

namespace postgresql {
    class Connection;

    /**
     * Forward-only streaming cursor over a PostgreSQL query result.
     * Uses libpq single-row mode, so rows arrive one by one instead of the
     * whole result being buffered. The underlying Connection must outlive the
     * cursor and must not run other queries while the cursor is unfinished;
     * the destructor drains any pending results to leave the connection clean.
     */
    class Cursor {
    private:
        PGconn *m_conn;
        bool m_active;

        /**
         * Consumes all remaining results of the streaming query.
         */
        void drain();

    public:
        /**
         * Sends the query in single-row mode.
         * @param connection the PostgreSQL connection (must outlive the cursor)
         * @param query the SQL query with $1, $2, ... placeholders
         * @param params the parameter values to bind
         * @throws db::SQLException if sending the query or switching to
         *         single-row mode fails
         */
        explicit Cursor(const Connection &connection, const std::string &query,
                        const std::vector<std::string> &params);

        Cursor(Cursor &cursor) = delete;

        /** Destructor, drains pending results if the stream is unfinished. */
        ~Cursor();

        /**
         * Fetches up to maxRows further rows from the query.
         * @param maxRows maximum number of rows to fetch (must be > 0)
         * @return the fetched rows; fewer rows than requested means the
         *         result set is exhausted
         * @throws db::SQLException on server-side failure
         */
        [[nodiscard]] std::vector<db::Arguments> fetchNext(std::size_t maxRows);
    };
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_CURSOR_H
