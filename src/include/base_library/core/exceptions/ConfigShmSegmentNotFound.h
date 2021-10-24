#pragma once

#include <exception>
#include <string>

class ConfigShmSegmentNotFound : public std::exception {
 private:
  std::string name;
  std::string message;

 public:
  explicit ConfigShmSegmentNotFound(const std::string_view &name)
      : name(name),
        message(this->name + ": segment not defined in configuration") {}

  [[nodiscard]] const std::string &getName() const { return name; }

  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  };
};
