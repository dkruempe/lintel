#ifndef CPP_SYSTEM_LIBRARY_SHMSEGMENTNOTFOUND_H
#define CPP_SYSTEM_LIBRARY_SHMSEGMENTNOTFOUND_H

#include <exception>
#include <string>

class ShmSegmentNotFound : public std::exception {
private:
  std::string name;
  std::string message;

public:
  explicit ShmSegmentNotFound(const std::string_view &name)
      : name(name),
        message(this->name +
                ": segment not constructed in SharedMemoryService") {}

  [[nodiscard]] const std::string &getName() const { return name; }

  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  };
};

#endif // CPP_SYSTEM_LIBRARY_SHMSEGMENTNOTFOUND_H
