#include "base_library/core/persistence/Arguments.h"

namespace db {
    void Arguments::add(Argument &argument) {
        argument.setConnectionType(m_connectionType);
        m_arguments.push_back(argument);
        m_argumentsMap.insert({argument.getName(), argument});
    }

    std::size_t Arguments::size() const { return m_arguments.size(); }

    Argument &Arguments::of(std::size_t iterator) {
        return m_arguments.at(iterator);
    }

    const Argument &Arguments::of(std::size_t iterator) const {
        return m_arguments.at(iterator);
    }

    const Argument &Arguments::of(const std::string &name) const {
        return m_argumentsMap.at(name);
    }

    bool Arguments::empty() const { return size() == 0; }

    Arguments::Arguments(ConnectionType connectionType)
            : m_connectionType(connectionType) {}

    Argument::Argument(std::string value, std::string name)
            : m_value(std::move(value)), m_name(std::move(name)) {}

    const std::string &Argument::getValue() const { return m_value; }

    const std::string &Argument::getName() const { return m_name; }

    std::ostream &operator<<(std::ostream &os, const db::Argument &argument) {
        os << "m_value: " << argument.m_value << " m_name: " << argument.m_name;
        return os;
    }

    void Argument::setConnectionType(ConnectionType connectionType) {
        m_connectionType = connectionType;
    }
}  // namespace db