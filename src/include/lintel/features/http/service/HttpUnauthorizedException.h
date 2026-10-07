#ifndef LINTEL_HTTPUNAUTHORIZEDEXCEPTION_H
#define LINTEL_HTTPUNAUTHORIZEDEXCEPTION_H

#include <exception>

/** Exception thrown when a user is not authorized to access a resource */
class HttpUnauthorizedException : public std::exception {
public:
    HttpUnauthorizedException() = default;

    [[nodiscard]] const char *what() const noexcept override {
        return "Unauthorized Access to called procedure!";
    }
};

#endif  // LINTEL_HTTPUNAUTHORIZEDEXCEPTION_H
