#ifndef CPP_BASE_LIBRARY_HISTORYSERVICEENTRY_H
#define CPP_BASE_LIBRARY_HISTORYSERVICEENTRY_H

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

#include "base_library/features/base/configuration/Entry.h"

/** Configuration entry for the history service */
class HistoryServiceEntry : public Entry {
private:
    /** The process name that owns the history service */
    const std::string m_processName;
    /** The name of the message queue used to ship history entries */
    const std::string m_queueName;
    /** The maximum number of messages the queue can hold */
    const int32_t m_maxMessages;

public:
    /** Construct a history service entry
     * @param component The configuration component name
     * @param processName The owning process name
     * @param queueName The name of the history message queue
     * @param maxMessages The maximum message count */
    HistoryServiceEntry(const std::string_view &component, std::string processName,
                 std::string queueName, int32_t maxMessages);

    /** Get the owning process name
     * @return The process name */
    [[nodiscard]] const std::string &get_process_name() const;

    /** Get the history message queue name
     * @return The queue name */
    [[nodiscard]] const std::string &get_queue_name() const;

    /** Get the maximum message count
     * @return The max messages */
    [[nodiscard]] constexpr int32_t get_max_messages() const { return m_maxMessages; }

    /** Stream insertion operator
     * @param os The output stream
     * @param entry The entry to output
     * @return The output stream */
    friend std::ostream &operator<<(std::ostream &os, const HistoryServiceEntry &entry);
};

#endif  // CPP_BASE_LIBRARY_HISTORYSERVICEENTRY_H
