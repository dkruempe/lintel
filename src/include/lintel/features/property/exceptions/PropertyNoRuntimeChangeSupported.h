#pragma once

#include <fmt/format.h>

#include <exception>
#include <string>
#include <utility>

#include "lintel/features/property/models/PropertyBase.h"

/** Exception thrown when a runtime change is attempted on a non-runtime-changeable property */
class PropertyNoRuntimeChangeSupported : public std::exception {
private:
    std::shared_ptr<PropertyBase> m_property;
    std::string m_message;

public:
    PropertyNoRuntimeChangeSupported() = delete;

    /** @param tempProperty the property that does not support runtime changes */
    explicit PropertyNoRuntimeChangeSupported(
            std::shared_ptr<PropertyBase> tempProperty)
            : m_property(std::move(tempProperty)),
              m_message(fmt::format("{} no runtime change allowed",
                                    m_property->toString())) {}

    [[nodiscard]] const char *what() const noexcept override {
        return m_message.c_str();
    }

    /** @return the property that caused the exception */
    [[nodiscard]] const std::shared_ptr<PropertyBase> &getProperty() {
        return m_property;
    }
};
