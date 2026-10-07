#pragma once

#include <exception>
#include <filesystem>

/** Exception thrown when the logger service is accessed before initialization */
class LoggerServiceNotInitialized : public std::exception {
private:
    const std::string m_message;

public:
    explicit LoggerServiceNotInitialized()
            : m_message(
            "LoggerService not initialized. Please call Marco DECLARE_LOGGER") {
    }

    [[nodiscard]] const char *what() const noexcept override {
        return m_message.c_str();
    }
};
