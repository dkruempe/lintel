#ifndef CPP_BASE_LIBRARY_MESSAGE_H
#define CPP_BASE_LIBRARY_MESSAGE_H

#include <ostream>
#include <string>

class Message {
 public:
  virtual std::string serialize() = 0;

  Message() = default;

  virtual ~Message() = default;

  friend std::ostream& operator<<(std::ostream& os, const Message& message) {
    return os;
  }
};

#endif  // CPP_BASE_LIBRARY_MESSAGE_H
