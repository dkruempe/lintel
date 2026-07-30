#ifndef CPP_BASE_LIBRARY_SQLITE_RESULT_H
#define CPP_BASE_LIBRARY_SQLITE_RESULT_H

#include <string>
#include <vector>

#include "base_library/core/persistence/Arguments.h"

namespace sqlite {
    /**
     * Wraps an SQLite query result with row/column access.
     */
    class Result {
    private:
        std::vector<db::Arguments> m_arguments;

    public:
        /**
         * Adds a row of arguments to the result.
         * @param arguments the row arguments
         */
        void add(const db::Arguments &arguments);

        /**
         * Retrieves a value at the given row and column.
         * @param row the row index
         * @param attribute the column index
         * @return the value as a string
         */
        [[nodiscard]] std::string getValue(int row, int attribute);

        /** @return the number of columns in the result */
        [[nodiscard]] int getNumOfAttributes() const;

        /** @return the number of rows in the result */
        [[nodiscard]] int getSize() const;

        /**
         * Accesses a row's arguments (mutable).
         * @param pos the row index
         * @return reference to the Arguments for that row
         */
        [[nodiscard]] db::Arguments &of(std::size_t pos);
    };
}  // namespace sqlite

#endif  // CPP_BASE_LIBRARY_SQLITE_RESULT_H
