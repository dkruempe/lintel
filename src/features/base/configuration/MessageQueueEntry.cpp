#include "lintel/features/base/configuration/MessageQueueEntry.h"

MessageQueueEntry::MessageQueueEntry(const std::string_view &component,
                                     std::string _processName,
                                     std::string _messageQueueName,
                                     int32_t _maxMessages)
        : Entry(component),
          processName(std::move(_processName)),
          messageQueueName(std::move(_messageQueueName)),
          maxMessages(_maxMessages) {}

const std::string &MessageQueueEntry::get_process_name() const {
    return processName;
}

const std::string &MessageQueueEntry::get_message_queue_name() const {
    return messageQueueName;
}

std::ostream &operator<<(std::ostream &os, const MessageQueueEntry &entry) {
    os << static_cast<const Entry &>(entry)
       << " processName: " << entry.processName
       << " messageQueueName: " << entry.messageQueueName
       << " maxMessages: " << entry.maxMessages;
    return os;
}
