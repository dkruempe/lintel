#pragma once
#include <fmt/format.h>

#include <exception>
#include <string>
#include <utility>

#include "base_library/models/PropertyBase.h"

class PropertyNoRuntimeChangeSupported : public std::exception {
 private:
  std::shared_ptr<PropertyBase> property;
  std::string message;

 public:
  PropertyNoRuntimeChangeSupported() = delete;

  explicit PropertyNoRuntimeChangeSupported(std::shared_ptr<PropertyBase> tempProperty)
      : property(std::move(tempProperty)),
        message(fmt::format("{} no runtime change allowed", property->toString())) {}

  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  }
  [[nodiscard]] const std::shared_ptr<PropertyBase> &getProperty() {
    return property;
  }
};
