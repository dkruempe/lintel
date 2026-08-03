#ifndef CPP_BASE_LIBRARY_ARGUMENTS_H
#define CPP_BASE_LIBRARY_ARGUMENTS_H

#include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "base_library/core/persistence/ConnectionType.h"
#include "base_library/core/persistence/Serialization.h"
#include "base_library/core/services/StringifyService.h"

namespace db {
    /**
     * Represents a single named argument with a value and associated connection type.
     */
    class Argument {
    private:
        std::string m_value;
        std::string m_name;
        db::ConnectionType m_connectionType;

    public:
        /**
         * Constructs an argument.
         * @param value the argument value as a string
         * @param name the argument name
         */
        Argument(std::string value, std::string name);

        /** @return the raw string value */
        [[nodiscard]] const std::string &getValue() const;

        /** @return the argument name */
        [[nodiscard]] const std::string &getName() const;

        /**
         * Sets the connection type for typed deserialization.
         * @param connectionType the database connection type
         */
        void setConnectionType(ConnectionType connectionType);

        /**
         * Deserializes the value to the requested type based on the connection type.
         * @tparam TYPE the target type
         * @return the deserialized value
         */
        template<typename TYPE>
        TYPE getValue() const {
            switch (m_connectionType) {
                case ConnectionType::PostgreSQL:
                    return postgresql::Serialization<TYPE>::deserialize(m_value);
                case ConnectionType::SQLite:
                    return sqlite::Serialization<TYPE>::deserialize(m_value);
                case ConnectionType::UNDEFINED:
                    return StringifyService<TYPE>::deserializeFromString(m_value);
            }
        }

        /** Streams the argument value and name. */
        friend std::ostream &operator<<(std::ostream &os,
                                        const db::Argument &argument);
    };

    /**
     * A collection of Argument objects with random-access iteration and lookup by name.
     */
    class Arguments {
    private:
        // members
        std::vector<Argument> m_arguments;
        std::map<std::string, Argument> m_argumentsMap;
        ConnectionType m_connectionType;

        // iterator classes
        /**
         * Mutable random-access iterator over Arguments.
         */
        class Iterator {
        private:
            Arguments &m_arguments;
            std::size_t m_pos;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = Argument;
            using difference_type = std::ptrdiff_t;
            using pointer = Argument *;
            using reference = Argument &;

            /**
             * Constructs an iterator for the given Arguments container.
             * @param arguments the container to iterate over
             * @param end if true, positions at the end; otherwise at the beginning
             */
            explicit Iterator(Arguments &arguments, bool end = false)
                    : m_arguments(arguments),
                      m_pos(end ? static_cast<std::size_t>(m_arguments.size()) : 0) {}

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

            reference operator*() { return m_arguments.of(m_pos); }
        };

        /**
         * Const random-access iterator over Arguments.
         */
        class ConstIterator {
        private:
            const Arguments &m_arguments;
            std::size_t m_pos;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = Argument;
            using difference_type = std::ptrdiff_t;
            using pointer = const Argument *;
            using reference = const Argument &;

            /**
             * Constructs a const iterator for the given Arguments container.
             * @param arguments the container to iterate over
             * @param end if true, positions at the end; otherwise at the beginning
             */
            explicit ConstIterator(const Arguments &arguments, bool end = false)
                    : m_arguments(arguments),
                      m_pos(end ? static_cast<std::size_t>(m_arguments.size()) : 0) {}

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

            reference operator*() const { return m_arguments.of(m_pos); }
        };

    public:
        /**
         * Constructs an Arguments container with the given connection type.
         * @param connectionType the connection type used for typed deserialization
         */
        explicit Arguments(ConnectionType connectionType);

        /** @param argument the argument to add */
        void add(Argument &argument);

        /** @return number of arguments */
        [[nodiscard]] std::size_t size() const;

        /** @return true if there are no arguments */
        [[nodiscard]] bool empty() const;

        /**
         * Accesses an argument by position (mutable).
         * @param iterator the index
         * @return reference to the argument at that index
         */
        Argument &of(std::size_t iterator);

        /**
         * Accesses an argument by position (const).
         * @param iterator the index
         * @return const reference to the argument at that index
         */
        [[nodiscard]] const Argument &of(std::size_t iterator) const;

        /**
         * Looks up an argument by name.
         * @param name the argument name
         * @return const reference to the matching argument
         */
        [[nodiscard]] const Argument &of(const std::string &name) const;

        /** @return mutable iterator to the beginning */
        Iterator begin() { return Iterator(*this); }

        /** @return mutable iterator past the end */
        Iterator end() { return Iterator(*this, true); }

        /** @return const iterator to the beginning */
        [[nodiscard]] ConstIterator begin() const { return ConstIterator(*this); }

        /** @return const iterator past the end */
        [[nodiscard]] ConstIterator end() const { return ConstIterator(*this, true); }
    };
};  // namespace db

#endif  // CPP_BASE_LIBRARY_ARGUMENTS_H
