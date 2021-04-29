#pragma once
#include <fmt/format.h>

#include <exception>
#include <string>
#include <utility>

#include "base_library/features/property/models/PropertyBase.h"

class PropertyNoRuntimeChangeSupported : public std::exception {
 private:
  std::shared_ptr<PropertyBase> m_property;
  std::string m_message;

 public:
  PropertyNoRuntimeChangeSupported() = delete;

  explicit PropertyNoRuntimeChangeSupported(
      std::shared_ptr<PropertyBase> tempProperty)
      : m_property(std::move(tempProperty)),
        m_message(fmt::format("{} no runtime change allowed",
                              m_property->toString())) {}

  [[nodiscard]] const char *what() const noexcept override {
    return m_message.c_str();
  }
  [[nodiscard]] const std::shared_ptr<PropertyBase> &getProperty() {
    return m_property;
  }
};
