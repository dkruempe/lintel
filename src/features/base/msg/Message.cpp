#include "base_library/features/base/msg/Message.h"

Message::Message(const std::string &receiver, const std::string &sender) {
    std::strncpy(m_receiver, receiver.c_str(), std::strlen(m_receiver));
    std::strncpy(m_sender, sender.c_str(), std::strlen(m_sender));
    std::memset(m_content, 0, sizeof(char) * std::strlen(m_content));
    auto now = std::chrono::system_clock::now();
    m_sendTime = std::chrono::system_clock::to_time_t(now);
}

const char *Message::receiverOf() const { return m_receiver; }

const char *Message::senderOf() const { return m_sender; }

std::chrono::time_point<std::chrono::system_clock> Message::sendTimeOf() const {
    return std::chrono::system_clock::from_time_t(m_sendTime);
}