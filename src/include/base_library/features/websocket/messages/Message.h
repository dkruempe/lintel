#ifndef CPP_BASE_LIBRARY_MESSAGE_H
#define CPP_BASE_LIBRARY_MESSAGE_H

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <ostream>
#include <string>
#include <functional>
#include <memory>

class Message {
 public:
  typedef rapidjson::GenericValue<rapidjson::UTF8<>,
                                  rapidjson::MemoryPoolAllocator<>>* jsonType;

  typedef std::function<std::shared_ptr<Message>(jsonType)> create_t;

  enum TYPE { REQUEST, RESPONSE, NOTIFICATION };
  virtual std::string serialize() = 0;

  virtual void serialize(
      rapidjson::Writer<rapidjson::StringBuffer>& writer) = 0;

  explicit Message(TYPE type) : m_type(type) {}

  virtual ~Message() = default;

  TYPE getType() { return m_type; }

  bool isRequest() { return m_type == REQUEST; }

  bool isNotification() { return m_type == NOTIFICATION; }

  bool isResponse() { return m_type == RESPONSE; }

  virtual create_t createFunctionOf() = 0;

 private:
  TYPE m_type;
};

#endif  // CPP_BASE_LIBRARY_MESSAGE_H
