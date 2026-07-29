#include "base_library/features/base/msg/Message.h"

#include <cstring>
#include <algorithm>

Message::Message(const std::string &receiver, const std::string &sender) {
    auto len = std::min(receiver.size(), sizeof(m_receiver) - 1);
    std::copy(receiver.begin(), receiver.begin() + static_cast<std::ptrdiff_t>(len), std::begin(m_receiver));
    *(std::begin(m_receiver) + static_cast<std::ptrdiff_t>(len)) = '\0';
    len = std::min(sender.size(), sizeof(m_sender) - 1);
    std::copy(sender.begin(), sender.begin() + static_cast<std::ptrdiff_t>(len), std::begin(m_sender));
    *(std::begin(m_sender) + static_cast<std::ptrdiff_t>(len)) = '\0';
    std::fill(std::begin(m_content), std::end(m_content), '\0');
    auto now = std::chrono::system_clock::now();
    m_sendTime = std::chrono::system_clock::to_time_t(now);
}

const char *Message::receiverOf() const { return &m_receiver[0]; }

const char *Message::senderOf() const { return &m_sender[0]; }

std::chrono::time_point<std::chrono::system_clock> Message::sendTimeOf() const {
    return std::chrono::system_clock::from_time_t(m_sendTime);
}