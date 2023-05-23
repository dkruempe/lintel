#include "base_library/features/base/configuration/MessageQueueEntry.h"

MessageQueueEntry::MessageQueueEntry(const std::string_view &component,
                                     std::string processName,
                                     std::string messageQueueName,
                                     int32_t maxMessages, bool removeOnShutdown)
        : Entry(component),
          processName(std::move(processName)),
          messageQueueName(std::move(messageQueueName)),
          maxMessages(maxMessages),
          removeOnShutdown(removeOnShutdown) {}

const std::string &MessageQueueEntry::get_process_name() const {
    return processName;
}

const std::string &MessageQueueEntry::get_message_queue_name() const {
    return messageQueueName;
}

int32_t MessageQueueEntry::get_max_messages() const { return maxMessages; }

bool MessageQueueEntry::get_remove_on_shutdown() const {
    return removeOnShutdown;
}

std::ostream &operator<<(std::ostream &os, const MessageQueueEntry &entry) {
    os << static_cast<const Entry &>(entry)
       << " processName: " << entry.processName
       << " messageQueueName: " << entry.messageQueueName
       << " maxMessages: " << entry.maxMessages
       << " removeOnShutdown: " << entry.removeOnShutdown;
    return os;
}
