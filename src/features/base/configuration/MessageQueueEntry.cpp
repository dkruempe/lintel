#include "base_library/features/base/configuration/MessageQueueEntry.h"

MessageQueueEntry::MessageQueueEntry(const std::string_view &component,
                                     std::string processName,
                                     std::string messageQueueName,
                                     int32_t maxMessages)
        : Entry(component),
          processName(std::move(processName)),
          messageQueueName(std::move(messageQueueName)),
          maxMessages(maxMessages) {}

const std::string &MessageQueueEntry::get_process_name() const {
    return processName;
}

const std::string &MessageQueueEntry::get_message_queue_name() const {
    return messageQueueName;
}

int32_t MessageQueueEntry::get_max_messages() const { return maxMessages; }

std::ostream &operator<<(std::ostream &os, const MessageQueueEntry &entry) {
    os << static_cast<const Entry &>(entry)
       << " processName: " << entry.processName
       << " messageQueueName: " << entry.messageQueueName
       << " maxMessages: " << entry.maxMessages;
    return os;
}
