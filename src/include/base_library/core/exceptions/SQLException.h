#ifndef CPP_BASE_LIBRARY_SQLEXCEPTION_H
#define CPP_BASE_LIBRARY_SQLEXCEPTION_H

#include <exception>
#include <string>

namespace db {
    class SQLException : public std::exception {
    private:
        std::string m_message;

    public:
        SQLException() = delete;

        explicit SQLException(std::string message) : m_message(std::move(message)) {}

        [[nodiscard]] const char *what() const noexcept override {
            return m_message.c_str();
        }
    };
}  // namespace db

#endif  // CPP_BASE_LIBRARY_SQLEXCEPTION_H
