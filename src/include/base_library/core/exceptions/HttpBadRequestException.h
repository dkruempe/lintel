#ifndef CPP_BASE_LIBRARY_HTTPBADREQUESTEXCEPTION_H
#define CPP_BASE_LIBRARY_HTTPBADREQUESTEXCEPTION_H

#include <exception>

/** Exception thrown when a request body cannot be parsed as a valid JSON object */
class HttpBadRequestException : public std::exception {
public:
    HttpBadRequestException() = default;

    [[nodiscard]] const char *what() const noexcept override {
        return "Invalid request body";
    }
};

#endif  // CPP_BASE_LIBRARY_HTTPBADREQUESTEXCEPTION_H
