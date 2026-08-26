#ifndef CPP_BASE_LIBRARY_POSTGRESQL_CURSOR_H
#define CPP_BASE_LIBRARY_POSTGRESQL_CURSOR_H

#include <libpq-fe.h>

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"

namespace postgresql {
    class Connection;

    /**
     * Forward-only cursor over a PostgreSQL query result.
     *
     * Two modes:
     *  - Streaming (fetchSize = 0): uses PQsetSingleRowMode so rows arrive
     *    one by one; the server executes the full query and the client
     *    buffers nothing.  Good for moderate result sets.
     *  - Server-side (fetchSize > 0): DECLARE CURSOR / FETCH / CLOSE.
     *    The server keeps state between FETCH calls, only transferring
     *    @p fetchSize rows per round-trip.  Enables FOR UPDATE locking
     *    across multiple fetches.
     *
     * The underlying Connection must outlive the cursor and must not run
     * other queries while the cursor is active.  The destructor closes
     * (server-side) or drains (streaming) to leave the connection clean.
     */
    class Cursor {
    private:
        PGconn *m_conn;
        bool m_active;
        bool m_serverSide;
        std::string m_cursorName;
        std::size_t m_fetchSize;

        /**
         * Consumes all remaining results of the streaming query.
         */
        void drain();

        /**
         * Sends a FETCH command and returns the resulting rows.
         * Used by the server-side path in fetchNext().
         */
        std::vector<db::Arguments> fetchFromServer(std::size_t maxRows);

        /**
         * Closes the server-side cursor (CLOSE <name>).
         */
        void close();

    public:
        /**
         * Streaming constructor (single-row mode, no DECLARE CURSOR).
         * @param connection the PostgreSQL connection (must outlive the cursor)
         * @param query the SQL query with $1, $2, ... placeholders
         * @param params the parameter values to bind
         * @throws db::SQLException if sending the query or switching to
         *         single-row mode fails
         */
        explicit Cursor(const Connection &connection, const std::string &query,
                        const std::vector<std::string> &params);

        /**
         * Server-side cursor constructor (DECLARE CURSOR / FETCH / CLOSE).
         * @param connection the PostgreSQL connection (must outlive the cursor)
         * @param query the SQL query with $1, $2, ... placeholders
         * @param params the parameter values to bind
         * @param fetchSize number of rows per FETCH (must be > 0)
         * @param cursorName the cursor name; if empty a name is generated
         * @param noScroll if true the cursor is declared NO SCROLL
         * @throws db::SQLException if DECLARE CURSOR fails
         */
        explicit Cursor(const Connection &connection, const std::string &query,
                        const std::vector<std::string> &params,
                        std::size_t fetchSize, const std::string &cursorName,
                        bool noScroll);

        Cursor(Cursor &cursor) = delete;

        /** Destructor, closes or drains pending results. */
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
