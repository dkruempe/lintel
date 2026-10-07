#ifndef LINTEL_HTTPBADREQUESTEXCEPTION_H
#define LINTEL_HTTPBADREQUESTEXCEPTION_H

#include <exception>

/** Exception thrown when a request body cannot be parsed as a valid JSON object */
class HttpBadRequestException : public std::exception {
public:
    HttpBadRequestException() = default;

    [[nodiscard]] const char *what() const noexcept override {
        return "Invalid request body";
    }
};

#endif  // LINTEL_HTTPBADREQUESTEXCEPTION_H
