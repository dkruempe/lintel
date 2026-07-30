#pragma once

#include <exception>
#include <string>

/** Exception thrown when a shared memory segment is not found in configuration */
class ConfigShmSegmentNotFound : public std::exception {
private:
    std::string name;
    std::string message;

public:
    /** @param name the name of the segment that was not found */
    explicit ConfigShmSegmentNotFound(const std::string_view &name)
            : name(name),
              message(this->name + ": segment not defined in configuration") {}

    /** @return the segment name */
    [[nodiscard]] const std::string &getName() const { return name; }

    [[nodiscard]] const char *what() const noexcept override {
        return message.c_str();
    };
};
