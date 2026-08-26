#ifndef CPP_BASE_LIBRARY_CURSOR_H
#define CPP_BASE_LIBRARY_CURSOR_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"
#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/postgresql/Cursor.h"
#include "base_library/core/persistence/sqlite3/Cursor.h"

namespace db {
    /**
     * Forward-only cursor over a query result.
     *
     * Two modes controlled by fetchSize:
     *  - fetchSize == 0: client-side streaming (PQsetSingleRowMode on PG).
     *  - fetchSize > 0:  server-side cursor (DECLARE/FETCH/CLOSE on PG,
     *                    client-side prefetch buffer on SQLite).  Default.
     *
     * The cursor is single-pass: iterate it once with begin()/end(), or
     * consume it with fetchNext().  The underlying Connection must outlive
     * the cursor.
     *
     * Configuration (setFetchSize / setCursorName / setNoScroll) must be
     * called before the first call to begin() or fetchNext().
     */
    class Cursor {
    private:
        std::shared_ptr<postgresql::Cursor> m_cursor = nullptr;
        std::shared_ptr<sqlite::Cursor> m_cursorSQLite = nullptr;

        /** Currently buffered batch of rows */
        std::vector<db::Arguments> m_batch;
        /** Position within the current batch */
        std::size_t m_pos = 0;
        bool m_started = false;
        bool m_exhausted = false;

        /** Fetch size: rows per backend round-trip (0 = streaming) */
        std::size_t m_fetchSize = 100;
        /** Server-side cursor name (auto-generated if empty) */
        std::string m_cursorName;
        /** NO SCROLL flag for server-side cursor */
        bool m_noScroll = true;

        /** Lazy-init state: query, params, and connection handles */
        bool m_needsInit = false;
        std::string m_query;
        std::vector<std::string> m_params;
        std::shared_ptr<postgresql::Connection> m_pgConn;
        std::shared_ptr<sqlite::Connection> m_sqliteConn;

        /**
         * Starts the stream by creating the backend cursor and fetching
         * the first batch (no-op if started).
         */
        void start();

        /**
         * Creates the backend cursor from stored query/params/connection.
         * Called once by start() when m_needsInit is true.
         */
        void initBackendCursor();

        /**
         * Discards the current batch and fetches the next one.
         */
        void fillBatch();

        /** @return true if the current batch still has an unread row */
        [[nodiscard]] bool hasCurrent() const;

        /** @return the row at the current position */
        [[nodiscard]] const db::Arguments &currentOf() const;

        /**
         * Advances to the next row, fetching further batches as needed.
         */
        void advance();

    public:
        /**
         * Mutable forward iterator over streamed rows.
         * Single-pass input-iterator semantics: copies share the stream
         * position, so only one iterator should be advanced at a time and
         * comparison is only meaningful against end().
         */
        class Iterator {
        private:
            Cursor &m_cursor;
            bool m_end;

            /** @return true if this iterator has reached (or marks) the end */
            [[nodiscard]] bool reachedEnd() const;

        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = Arguments;
            using difference_type = std::ptrdiff_t;
            using pointer = const Arguments *;
            using reference = const Arguments &;

            /**
             * Constructs an iterator for the given cursor.
             * @param cursor the cursor to iterate over
             * @param end if true, positions at the logical end; otherwise at
             *            the cursor's current row
             */
            explicit Iterator(Cursor &cursor, bool end = false);

            Iterator &operator++();

            Iterator operator++(int);

            bool operator==(const Iterator &other) const;

            bool operator!=(const Iterator &other) const;

            reference operator*() const;
        };

        /** Default constructor, creates an exhausted cursor. */
        Cursor() = default;

        /**
         * Sets the number of rows fetched per backend round-trip.
         *  0 = client-side streaming (PQsetSingleRowMode on PG, no DECLARE CURSOR)
         * >0 = server-side cursor (DECLARE/FETCH/CLOSE on PG, prefetch buffer on SQLite)
         * Default: 100 (server-side).
         * Must be called before begin() or fetchNext().
         */
        Cursor &setFetchSize(std::size_t rows);

        /**
         * Sets the server-side cursor name.
         * If empty (default), a unique name is auto-generated.
         * Must be called before begin() or fetchNext().
         */
        Cursor &setCursorName(const std::string &name);

        /**
         * Sets the NO SCROLL flag on the server-side cursor.
         * Default: true (NO SCROLL).
         * Must be called before begin() or fetchNext().
         */
        Cursor &setNoScroll(bool enable);

        /**
         * Fetches up to maxRows further rows from the stream.
         * @param maxRows maximum number of rows to fetch (must be > 0)
         * @return the fetched rows; fewer rows than requested means the
         *         result set is exhausted
         */
        std::vector<db::Arguments> fetchNext(std::size_t maxRows);

        /** Starts the stream and returns an iterator to the first row. */
        Iterator begin();

        /** @return iterator marking the end of the stream */
        Iterator end();

    private:
        friend class Statement;

        /**
         * Constructs a cursor wrapping exactly one backend cursor.
         */
        Cursor(std::shared_ptr<postgresql::Cursor> cursor,
               std::shared_ptr<sqlite::Cursor> cursorSQLite);
    };
}  // namespace db

#endif  // CPP_BASE_LIBRARY_CURSOR_H
