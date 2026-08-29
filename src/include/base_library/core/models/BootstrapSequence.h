#ifndef CPP_BASE_LIBRARY_BOOTSTRAP_H
#define CPP_BASE_LIBRARY_BOOTSTRAP_H

#include <magic_enum/magic_enum.hpp>
#include <set>
#include <string>

/**
 * Defines the bootstrap sequence priority for property repository types.
 */
class BootstrapSequence {
public:
    // value is defining priority of property repository type
    enum Value {
        Undefined = 0,
        SingleInstance = 1,
        Database = 2,
        MessageQueue = 3,
        VirtualGroups = 4,
        SharedMemory = 5,
        AdminUser = 6
    };

    /** Default constructor, initializes to Undefined. */
    BootstrapSequence() = default;

    /**
     * Constructs from a Value enum.
     * @param value the bootstrap priority value
     */
    constexpr BootstrapSequence(Value value) : m_value(value) {}

    /**
     * Constructs from a string representation of a Value.
     * @param enumName string matching a Value name, defaults to Undefined if not found
     */
    constexpr explicit BootstrapSequence(std::string_view enumName)
            : m_value(magic_enum::enum_cast<Value>(enumName).value_or(Undefined)) {}

    /** Implicit conversion to Value. */
    constexpr operator Value() const { return m_value; }

    explicit operator bool() = delete;

    /** @return set of all defined Value entries */
    static std::set<Value> values() {
        auto values = magic_enum::enum_values<Value>();
        return std::set<Value>(values.begin(), values.end());
    }

    /** @return string representation of the current value */
    std::string toString() const {
        return std::string(magic_enum::enum_name<>(m_value));
    }

    /** Streams the string representation of the bootstrap sequence value. */
    friend std::ostream &operator<<(std::ostream &os,
                                    const BootstrapSequence &bootstrap) {
        os << magic_enum::enum_name<>(bootstrap.m_value);
        return os;
    }

private:
    Value m_value;
};

#endif  // CPP_BASE_LIBRARY_BOOTSTRAP_H
