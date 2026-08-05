#ifndef CPP_BASE_LIBRARY_SERIALIZATION_H
#define CPP_BASE_LIBRARY_SERIALIZATION_H

#include <exception>
#include <memory>

#include "base_library/core/persistence/postgresql/Serialization.h"
#include "base_library/core/persistence/sqlite3/Serialization.h"
#include "base_library/core/configuration/DatabaseConnectionEntry.h"

namespace db {
    /**
     * Dispatcher for type serialization that delegates to the appropriate
     * backend (PostgreSQL or SQLite) based on the connection entry type.
     * @tparam T the type to serialize/deserialize
     */
    template<typename T>
    class Serialization {
    public:
        /**
         * Serializes a value to a string using the backend for the given connection type.
         * @param type the value to serialize
         * @param connectionEntry the connection entry specifying the backend
         * @return the serialized string
         * @throws std::runtime_error if the connection type is not supported
         */
        static std::string serialize(
                T type, const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry) {
            switch (connectionEntry->getType()) {
                case ConnectionType::SQLite:
                    return sqlite::Serialization<T>::serialize(type);
                case ConnectionType::PostgreSQL:
                    return postgresql::Serialization<T>::serialize(type);
                default:
                    throw std::runtime_error("not supported connection type");
            }
        }

        /**
         * Deserializes a string to a value using the backend for the given connection type.
         * @param value the string to deserialize
         * @param connectionEntry the connection entry specifying the backend
         * @return the deserialized value
         * @throws std::runtime_error if the connection type is not supported
         */
        static T deserialize(
                const std::string &value,
                const std::shared_ptr<DatabaseConnectionEntry> &connectionEntry) {
            switch (connectionEntry->getType()) {
                case ConnectionType::SQLite:
                    return sqlite::Serialization<T>::deserialize(value);
                case ConnectionType::PostgreSQL:
                    return postgresql::Serialization<T>::deserialize(value);
                default:
                    throw std::runtime_error("not supported connection type");
            }
        }
    };
}  // namespace db

#endif  // CPP_BASE_LIBRARY_SERIALIZATION_H
