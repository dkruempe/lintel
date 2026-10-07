#ifndef LINTEL_MESSAGEQUEUES_H
#define LINTEL_MESSAGEQUEUES_H

#include <set>
#include <map>
#include <string_view>
#include <magic_enum/magic_enum.hpp>

/**
 * list of all available MessageQueues
 */
class MessageQueues {
public:
    /** Available message queue identifiers. */
    enum Value {
        UNDEFINED = -1, /**< Undefined / unset */
        HISTORY = 0     /**< History message queue */
    };

    /** Default constructor (sets value to UNDEFINED). */
    MessageQueues() = default;

    /** @param value enum value */
    constexpr MessageQueues(Value value) : m_value(value) {}

    /**
     * Construct from a string representation of the enum name.
     * @param enumName enum name string
     */
    constexpr explicit MessageQueues(std::string_view enumName)
            : m_value(magic_enum::enum_cast<Value>(enumName).value_or(UNDEFINED)) {}

    /** @return the underlying enum value */
    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    /** @return all defined enum values */
    static std::set<Value> values() {
        auto values = magic_enum::enum_values<Value>();
        return std::set<Value>(values.begin(), values.end());
    }

    /** @return string representation of the enum value */
    std::string toString() const {
        return std::string(magic_enum::enum_name<>(m_value));
    }

    /**
     * @return the message queue name for this enum value
     * @throws std::runtime_error if value is UNDEFINED
     */
    std::string getMessageQueueName() const {
        if (m_value == Value::UNDEFINED) {
            throw std::runtime_error("Message Queue undefined => no name available");
        }
        return m_valueToQueueName.at(m_value);
    }

    friend std::ostream &operator<<(std::ostream &os,
                                    const MessageQueues &connectionType) {
        os << magic_enum::enum_name<>(connectionType.m_value);
        return os;
    }

private:
    inline static std::map<Value, std::string> m_valueToQueueName =
            {{Value::HISTORY, "history"}};
    Value m_value;
};

#endif //LINTEL_MESSAGEQUEUES_H
