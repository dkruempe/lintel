#ifndef CPP_BASE_LIBRARY_HTTPUNAUTHORIZEDEXCEPTION_H
#define CPP_BASE_LIBRARY_HTTPUNAUTHORIZEDEXCEPTION_H

#include <exception>

class HttpUnauthorizedException : public std::exception {
 public:
  HttpUnauthorizedException() = default;

  [[nodiscard]] const char *what() const noexcept override {
    return "Unauthorized Access to called procedure!";
  }
};

#endif  // CPP_BASE_LIBRARY_HTTPUNAUTHORIZEDEXCEPTION_H
