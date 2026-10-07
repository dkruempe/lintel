#ifndef LINTEL_SHMSEGMENTNOTFOUND_H
#define LINTEL_SHMSEGMENTNOTFOUND_H

#include <exception>
#include <string>

/** Exception thrown when a shared memory segment is not constructed in SharedMemoryService */
class ShmSegmentNotFound : public std::exception {
private:
    std::string name;
    std::string message;

public:
    /** @param name the name of the segment that was not found */
    explicit ShmSegmentNotFound(const std::string_view &name)
            : name(name),
              message(this->name +
                      ": segment not constructed in SharedMemoryService") {}

    /** @return the segment name */
    [[nodiscard]] const std::string &getName() const { return name; }

    [[nodiscard]] const char *what() const noexcept override {
        return message.c_str();
    };
};

#endif  // LINTEL_SHMSEGMENTNOTFOUND_H
