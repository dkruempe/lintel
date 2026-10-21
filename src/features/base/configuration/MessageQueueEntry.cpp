#include "lintel/features/base/configuration/MessageQueueEntry.h"

namespace lintel {
MessageQueueEntry::MessageQueueEntry(const std::string_view &component,
  std::string _processName,
  std::string _messageQueueName,
  int32_t _maxMessages)
  : Entry(component), processName(std::move(_processName)), messageQueueName(std::move(_messageQueueName)),
    maxMessages(_maxMessages)
{}

// The two std::optional<time_point> timestamps (started_at / last_heartbeat_at)
// share their type, so both constructors take a single MessageQueueOwnerColumns
// aggregate instead of five adjacent optionals.
MessageQueueEntry::MessageQueueEntry(const std::string_view &component,
  std::string _processName,
  std::string _messageQueueName,
  int32_t _maxMessages,
  MessageQueueOwnerColumns owner)
  : Entry(component), processName(std::move(_processName)), messageQueueName(std::move(_messageQueueName)),
    maxMessages(_maxMessages), m_ownerPid(owner.pid), m_ownerHost(std::move(owner.host)), m_startedAt(owner.startedAt),
    m_lastHeartbeatAt(owner.lastHeartbeatAt), m_ownerState(std::move(owner.state))
{}

const std::string &MessageQueueEntry::get_process_name() const { return processName; }

const std::string &MessageQueueEntry::get_message_queue_name() const { return messageQueueName; }

std::ostream &operator<<(std::ostream &os, const MessageQueueEntry &entry)
{
  os << static_cast<const Entry &>(entry) << " processName: " << entry.processName
     << " messageQueueName: " << entry.messageQueueName << " maxMessages: " << entry.maxMessages;
  return os;
}
}// namespace lintel
