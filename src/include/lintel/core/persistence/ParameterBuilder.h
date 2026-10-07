#ifndef LINTEL_PARAMETERBUILDER_H
#define LINTEL_PARAMETERBUILDER_H

#include <string>
#include <utility>
#include <vector>

#include "lintel/core/persistence/Serialization.h"

namespace db {
    /**
     * Builder for constructing a list of serialized parameter strings from typed values.
     */
    class ParameterBuilder {
    private:
        std::vector<std::string> m_parameters;
        std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

    public:
        /**
         * Constructs a ParameterBuilder with the given connection entry for serialization.
         * @param connectionEntry the database connection configuration entry
         */
        explicit ParameterBuilder(
                std::shared_ptr<DatabaseConnectionEntry> connectionEntry)
                : m_connectionEntry(std::move(connectionEntry)) {}

        /**
         * Adds a typed parameter value, serializing it for the configured connection type.
         * @tparam T the parameter type
         * @param t the parameter value
         * @return reference to this builder for chaining
         */
        template<typename T>
        ParameterBuilder &add(T t) {
            m_parameters.push_back(
                    db::Serialization<T>::serialize(t, m_connectionEntry));
            return *this;
        }

        /** @return the list of serialized parameter strings */
        [[nodiscard]] const std::vector<std::string> &build() const {
            return m_parameters;
        }

        /** Clears all accumulated parameters. */
        void clear() { m_parameters.clear(); }
    };
}  // namespace db

#endif  // LINTEL_PARAMETERBUILDER_H
