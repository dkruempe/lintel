#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEENTRY_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEENTRY_H

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

#include "base_library/features/base/configuration/Entry.h"

/** Configuration entry for a message queue */
class MessageQueueEntry : public Entry {
private:
    /** The process name that owns the queue */
    const std::string processName;
    /** The name of the message queue */
    const std::string messageQueueName;
    /** The maximum number of messages the queue can hold */
    const int32_t maxMessages;

public:
    /** Construct a message queue entry
     * @param component The configuration component name
     * @param processName The owning process name
     * @param messageQueueName The queue name
     * @param maxMessages The maximum message count */
    MessageQueueEntry(const std::string_view &component, std::string processName,
                      std::string messageQueueName, int32_t maxMessages);

    /** Get the process name
     * @return The process name */
    [[nodiscard]] const std::string &get_process_name() const;

    /** Get the message queue name
     * @return The queue name */
    [[nodiscard]] const std::string &get_message_queue_name() const;

    /** Get the maximum message count
     * @return The max messages */
    [[nodiscard]] constexpr int32_t get_max_messages() const { return maxMessages; }

    /** Stream insertion operator
     * @param os The output stream
     * @param entry The entry to output
     * @return The output stream */
    friend std::ostream &operator<<(std::ostream &os,
                                    const MessageQueueEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUEENTRY_H
