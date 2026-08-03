#ifndef CPP_BASE_LIBRARY_RESULT_H
#define CPP_BASE_LIBRARY_RESULT_H

#include <iterator>
#include <string>

#include "base_library/core/persistence/Arguments.h"
#include "base_library/core/persistence/Connection.h"
#include "base_library/core/persistence/postgresql/Result.h"
#include "base_library/core/persistence/sqlite3/Result.h"

namespace db {
    /**
     * Unified query result wrapping either a PostgreSQL or SQLite result.
     * Supports random-access iteration over rows, each row being an Arguments object.
     */
    class Result {
    private:
        std::shared_ptr<postgresql::Result> m_result = nullptr;
        std::shared_ptr<sqlite::Result> m_resultSQLite = nullptr;

    public:
        /**
         * Mutable random-access iterator over result rows.
         */
        class Iterator
                : public std::iterator<std::random_access_iterator_tag, Arguments,
                        std::ptrdiff_t, Arguments *, Arguments &> {
        private:
            Result &m_result;
            std::size_t m_pos;

        public:
            /**
             * Constructs an iterator for the given Result.
             * @param result the result to iterate over
             * @param end if true, positions at the end; otherwise at the beginning
             */
            explicit Iterator(Result &result, bool end = false)
                    : m_result(result),
                      m_pos(end ? static_cast<std::size_t>(m_result.getSize()) : 0) {}

            Iterator &operator++() {
                ++m_pos;
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++m_pos;
                return tmp;
            }

            bool operator==(Iterator other) const { return m_pos == other.m_pos; }

            bool operator!=(Iterator other) const { return !(*this == other); }

            reference operator*() { return m_result.of(m_pos); }
        };

        /**
         * Const random-access iterator over result rows.
         */
        class ConstIterator
                : public std::iterator<std::random_access_iterator_tag, Arguments,
                        std::ptrdiff_t, const Arguments *,
                        const Arguments &> {
        private:
            const Result &m_result;
            std::size_t m_pos;

        public:
            /**
             * Constructs a const iterator for the given Result.
             * @param result the result to iterate over
             * @param end if true, positions at the end; otherwise at the beginning
             */
            explicit ConstIterator(const Result &result, bool end = false)
                    : m_result(result),
                      m_pos(end ? static_cast<std::size_t>(m_result.getSize()) : 0) {}

            ConstIterator &operator++() {
                ++m_pos;
                return *this;
            }

            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++m_pos;
                return tmp;
            }

            bool operator==(ConstIterator other) const { return m_pos == other.m_pos; }

            bool operator!=(ConstIterator other) const { return !(*this == other); }

            reference operator*() const { return m_result.of(m_pos); }
        };

        /** @return mutable iterator to the first row */
        Iterator begin() { return Iterator(*this); }

        /** @return mutable iterator past the last row */
        Iterator end() { return Iterator(*this, true); }

        /** @return const iterator to the first row */
        ConstIterator begin() const { return ConstIterator(*this); }

        /** @return const iterator past the last row */
        ConstIterator end() const { return ConstIterator(*this, true); }

        /** Default constructor. */
        Result() = default;

        /** Destructor. */
        ~Result();

        /**
         * Constructs a result wrapping a PostgreSQL result.
         * @param result the PostgreSQL result shared pointer
         */
        explicit Result(std::shared_ptr<postgresql::Result> result);

        /**
         * Constructs a result wrapping an SQLite result.
         * @param result the SQLite result shared pointer
         */
        explicit Result(std::shared_ptr<sqlite::Result> result);

        /**
         * Retrieves a value at the given row and attribute.
         * @param row the row index
         * @param attribute the attribute (column) index
         * @return the value as a string
         */
        [[nodiscard]] std::string getValue(int row, int attribute) const;

        /**
         * Accesses a row as Arguments (mutable).
         * @param pos the row index
         * @return reference to the Arguments for that row
         */
        [[nodiscard]] Arguments &of(std::size_t pos);

        /**
         * Accesses a row as Arguments (const).
         * @param pos the row index
         * @return const reference to the Arguments for that row
         */
        [[nodiscard]] const Arguments &of(std::size_t pos) const;

        /** @return the number of columns/attributes in the result */
        [[nodiscard]] int getNumOfAttributes() const;

        /** @return the number of rows in the result */
        [[nodiscard]] int getSize() const;
    };
}  // namespace db

#endif  // CPP_BASE_LIBRARY_RESULT_H
