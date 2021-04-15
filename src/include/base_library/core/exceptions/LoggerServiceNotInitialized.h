#pragma once
#include <exception>
#include <filesystem>

class LoggerServiceNotInitialized : public std::exception {
private:
  const std::string message;

public:
  explicit LoggerServiceNotInitialized()
      : message(
            "LoggerService not initialized. Please call Marco DECLARE_LOGGER") {
  }

  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  }
};