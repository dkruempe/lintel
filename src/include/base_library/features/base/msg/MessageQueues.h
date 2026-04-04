#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUES_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUES_H

#include <set>
#include <map>
#include <string_view>
#include <magic_enum/magic_enum.hpp>

/**
 * list of all available MessageQueues
 */
class MessageQueues {
public:
    // value is defining priority of property repository type
    enum Value {
        UNDEFINED = -1,
        HISTORY = 0
    };

    MessageQueues() = default;

    constexpr MessageQueues(Value value) : m_value(value) {}

    constexpr explicit MessageQueues(std::string_view enumName)
            : m_value(magic_enum::enum_cast<Value>(enumName).value_or(UNDEFINED)) {}

    operator Value() const { return m_value; }

    explicit operator bool() = delete;

    static std::set<Value> values() {
        auto values = magic_enum::enum_values<Value>();
        return std::set<Value>(values.begin(), values.end());
    }

    std::string toString() const {
        return std::string(magic_enum::enum_name<>(m_value));
    }

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

#endif //CPP_BASE_LIBRARY_MESSAGEQUEUES_H
