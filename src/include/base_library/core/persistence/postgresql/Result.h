#ifndef CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H
#define CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H

#include <libpq-fe.h>

#include <string>

#include "base_library/core/persistence/Arguments.h"

namespace postgresql {
    /**
     * Wraps a PostgreSQL PGresult and provides row/column access.
     */
    class Result {
    private:
        PGresult *m_res;
        std::vector<db::Arguments> m_arguments;

    public:
        /**
         * Constructs a Result from a PGresult pointer, taking ownership.
         * @param res the PGresult pointer
         */
        explicit Result(PGresult *res);

        /** Destructor, frees the PGresult. */
        ~Result();

        /**
         * Checks whether the result has the given execution status.
         * @param status the expected ExecStatusType
         * @return true if the result status matches
         */
        [[nodiscard]] bool isState(ExecStatusType status) const;

        /**
         * Retrieves a value at the given row and column.
         * @param row the row index
         * @param attribute the column index
         * @return the value as a string
         */
        [[nodiscard]] std::string getValue(int row, int attribute) const;

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
}  // namespace postgresql

#endif  // CPP_BASE_LIBRARY_POSTGRESQL_RESULT_H
