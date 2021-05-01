#ifndef CPP_BASE_LIBRARY_MESSAGE_H
#define CPP_BASE_LIBRARY_MESSAGE_H

#include <ostream>
#include <string>

class Message {
 public:
  enum TYPE { REQUEST, RESPONSE, NOTIFICATION };
  virtual std::string serialize() = 0;

  explicit Message(TYPE type) : m_type(type) {}

  virtual ~Message() = default;

  TYPE getType() { return m_type; }

  bool isRequest() { return m_type == REQUEST; }

  bool isNotification() { return m_type == NOTIFICATION; }

  bool isResponse() { return m_type == RESPONSE; }

 private:
  TYPE m_type;
};

#endif  // CPP_BASE_LIBRARY_MESSAGE_H
