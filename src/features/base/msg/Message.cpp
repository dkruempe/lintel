#include "base_library/features/base/msg/Message.h"

#include <cstring>

Message::Message(const std::string &receiver, const std::string &sender) {
    std::strncpy(m_receiver, receiver.c_str(), sizeof(m_receiver) - 1);
    m_receiver[sizeof(m_receiver) - 1] = '\0';
    std::strncpy(m_sender, sender.c_str(), sizeof(m_sender) - 1);
    m_sender[sizeof(m_sender) - 1] = '\0';
    std::memset(m_content, 0, sizeof(m_content));
    auto now = std::chrono::system_clock::now();
    m_sendTime = std::chrono::system_clock::to_time_t(now);
}

const char *Message::receiverOf() const { return m_receiver; }

const char *Message::senderOf() const { return m_sender; }

std::chrono::time_point<std::chrono::system_clock> Message::sendTimeOf() const {
    return std::chrono::system_clock::from_time_t(m_sendTime);
}