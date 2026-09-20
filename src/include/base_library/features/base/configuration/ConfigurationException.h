#ifndef CPP_BASE_LIBRARY_CONFIGURATIONEXCEPTION_H
#define CPP_BASE_LIBRARY_CONFIGURATIONEXCEPTION_H

#include <cstdint>
#include <exception>
#include <string>
#include <utility>

/** Exception thrown when configuration parsing fails */
class ConfigurationException : public std::exception {
private:
    /** The name of the component that caused the error */
    std::string m_componentName;
    /** The error message */
    std::string m_message;
    /** The line number where the error occurred */
    int32_t m_line;
    /** The formatted error message */
    std::string m_printMessage;

public:
    /** Construct a configuration exception
     * @param componentName Name of the component
     * @param message Error description
     * @param line Line number of the error */
    ConfigurationException(std::string componentName, std::string message,
                           int32_t line)
            : m_componentName(std::move(componentName)),
              m_message(std::move(message)),
              m_line(line) {
        m_printMessage =
                m_componentName + ":" + std::to_string(m_line) + " -> " + m_message;
    }

    /** Get the error message
     * @return C-string describing the error */
    [[nodiscard]] const char *what() const noexcept override {
        return m_printMessage.c_str();
    }
};

#endif  // CPP_BASE_LIBRARY_CONFIGURATIONEXCEPTION_H
