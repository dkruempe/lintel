#ifndef PLC_PROPERTYREPOSITORYTYPE_H
#define PLC_PROPERTYREPOSITORYTYPE_H

#include <magic_enum/magic_enum.hpp>
#include <set>
#include <string>

/** Enum wrapper for property repository types with string conversion */
class PropertyRepositoryType {
public:
    // value is defining priority of property repository type
    enum Value {
        UNDEFINED = -1,
        DEFAULT = 0,
        FILE_REPOSITORY = 1,
        DATABASE_REPOSITORY = 2,
        SHM_REPOSITORY = 3,
    };

    PropertyRepositoryType() = default;

    constexpr PropertyRepositoryType(Value value) : m_value(value) {}

    /** @param enumName string matching an enum name */
    constexpr explicit PropertyRepositoryType(std::string_view enumName)
            : m_value(magic_enum::enum_cast<Value>(enumName).value_or(UNDEFINED)) {}

    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    /** @return all possible enum values */
    static std::set<Value> values() {
        auto values = magic_enum::enum_values<Value>();
        return std::set<Value>(values.begin(), values.end());
    }

    /** @return string representation of the enum value */
    std::string toString() const {
        return std::string(magic_enum::enum_name<>(m_value));
    }

    friend std::ostream &operator<<(
            std::ostream &os,
            const PropertyRepositoryType &propertyRepositoryPriority) {
        os << magic_enum::enum_name<>(propertyRepositoryPriority.m_value);
        return os;
    }

private:
    Value m_value;
};

#endif  // PLC_PROPERTYREPOSITORYTYPE_H
