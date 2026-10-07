#ifndef LOGGING_PROPERTYNOTFOUNDEXCEPTION_H
#define LOGGING_PROPERTYNOTFOUNDEXCEPTION_H

#include <fmt/format.h>

#include <exception>
#include <string>

/** Exception thrown when a requested property is not found */
class PropertyNotFoundException : public std::exception {
private:
    std::string m_name;
    std::string m_instanceName;
    std::string m_className;
    std::string m_processName;
    std::string m_message;

public:
    /** @param name property name
     *  @param instanceName instance name
     *  @param className class name
     *  @param processName process name */
    PropertyNotFoundException(const std::string &name,
                              const std::string &instanceName,
                              const std::string &className,
                              const std::string &processName)
            : m_name(name),
              m_instanceName(instanceName),
              m_className(className),
              m_processName(processName),
              m_message(fmt::format("Property<> with name: {} instanceName: {} "
                                    "className: {} processName: {} not found",
                                    name, instanceName, className, processName)) {}

    [[nodiscard]] const char *what() const noexcept override {
        return m_message.c_str();
    }

    /** @return the property name */
    [[nodiscard]] const std::string &getName() const { return m_name; }

    /** @return the instance name */
    [[nodiscard]] const std::string &getInstanceName() const {
        return m_instanceName;
    }

    /** @return the class name */
    [[nodiscard]] const std::string &getClassName() const { return m_className; }

    /** @return the process name */
    [[nodiscard]] const std::string &getProcessName() const {
        return m_processName;
    }
};

#endif  // LOGGING_PROPERTYNOTFOUNDEXCEPTION_H
