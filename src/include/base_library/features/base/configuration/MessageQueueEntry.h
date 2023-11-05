#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEENTRY_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEENTRY_H

#include <ostream>
#include <string>

#include "base_library/features/base/configuration/Entry.h"

class MessageQueueEntry : public Entry {
private:
    const std::string processName;
    const std::string messageQueueName;
    const int32_t maxMessages;

public:
    MessageQueueEntry(const std::string_view &component, std::string processName,
                      std::string messageQueueName, int32_t maxMessages);

    const std::string &get_process_name() const;

    const std::string &get_message_queue_name() const;

    int32_t get_max_messages() const;

    friend std::ostream &operator<<(std::ostream &os,
                                    const MessageQueueEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUEENTRY_H
