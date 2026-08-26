#ifndef CPP_BASE_LIBRARY_CURSOR_H
#define CPP_BASE_LIBRARY_CURSOR_H

#include <memory>
#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"
#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/postgresql/Cursor.h"
#include "base_library/core/persistence/sqlite3/Cursor.h"

namespace db {
    /**
     * Forward-only streaming cursor over a query result.
     * Unlike Result, rows are fetched lazily in batches from the database
     * instead of buffering the whole result set upfront.
     * The cursor is single-pass: iterate it once with begin()/end() like an
     * istream_iterator, or consume it with fetchNext(). The underlying
     * Connection must outlive the cursor.
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

        /** Number of rows fetched per backend round trip */
        static constexpr std::size_t BATCH_SIZE = 64;

        /**
         * Starts the stream by fetching the first batch (no-op if started).
         */
        void start();

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
