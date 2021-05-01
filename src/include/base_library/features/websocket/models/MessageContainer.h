#ifndef CPP_BASE_LIBRARY_MESSAGECONTAINER_H
#define CPP_BASE_LIBRARY_MESSAGECONTAINER_H

#include <memory>
#include <vector>

#include "base_library/features/websocket/messages/Message.h"

class MessageContainer {
 private:
  std::vector<std::shared_ptr<Message>> m_messages;
  std::vector<std::shared_ptr<Message>> m_errors;

 public:
  MessageContainer();

  void add(std::shared_ptr<Message> message);

  void addError(std::shared_ptr<Message> message);

  std::vector<std::shared_ptr<Message>> getMessages();

  std::vector<std::shared_ptr<Message>> getErrors();
};

#endif  // CPP_BASE_LIBRARY_MESSAGECONTAINER_H
