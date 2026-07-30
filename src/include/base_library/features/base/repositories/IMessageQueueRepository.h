#ifndef CPP_BASE_LIBRARY_IMESSAGEQUEUEREPOSITORY_H
#define CPP_BASE_LIBRARY_IMESSAGEQUEUEREPOSITORY_H

#include <memory>
#include <string>
#include <vector>

class MessageQueueEntry;

/**
 * Interface for message queue persistence operations.
 */
class IMessageQueueRepository {
public:
    virtual ~IMessageQueueRepository() = default;

    /**
     * @param name message queue name
     * @return the matching MessageQueueEntry
     */
    virtual MessageQueueEntry allMessageQueueNameOf(const std::string &name) = 0;

    /**
     * @param processName process name filter
     * @param messageQueueName queue name filter
     * @return matching entries
     */
    virtual std::vector<MessageQueueEntry> allOf(const std::string &processName, const std::string &messageQueueName) = 0;

    /**
     * @param processName filter by process name
     * @return all entries for the given process
     */
    virtual std::vector<MessageQueueEntry> allProcessNameOf(const std::string &processName) = 0;

    /**
     * Insert entries.
     * @param entries entries to insert
     */
    virtual void insertOf(const std::vector<MessageQueueEntry> &entries) = 0;

    /**
     * Delete entries.
     * @param entries entries to delete
     */
    virtual void deleteOf(const std::vector<MessageQueueEntry> &entries) = 0;
};

#endif  // CPP_BASE_LIBRARY_IMESSAGEQUEUEREPOSITORY_H
