#ifndef LINTEL_HTTPBADREQUESTEXCEPTION_H
#define LINTEL_HTTPBADREQUESTEXCEPTION_H

#include <exception>

namespace lintel {
/** Exception thrown when a request body cannot be parsed as a valid JSON object */
class HttpBadRequestException : public std::exception
{
public:
  HttpBadRequestException() = default;

  [[nodiscard]] const char *what() const noexcept override { return "Invalid request body"; }
};

}// namespace lintel
#endif// LINTEL_HTTPBADREQUESTEXCEPTION_H
