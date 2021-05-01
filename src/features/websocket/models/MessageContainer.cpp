#include "base_library/features/websocket/models/MessageContainer.h"

MessageContainer::MessageContainer() = default;

void MessageContainer::add(std::shared_ptr<Message> message) {
  m_messages.push_back(std::move(message));
}

void MessageContainer::addError(std::shared_ptr<Message> message) {
  m_errors.push_back(std::move(message));
}

std::vector<std::shared_ptr<Message>> MessageContainer::getErrors() {
  return m_errors;
}

std::vector<std::shared_ptr<Message>> MessageContainer::getMessages() {
  return m_messages;
}