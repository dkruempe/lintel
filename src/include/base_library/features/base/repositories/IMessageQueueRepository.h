#ifndef CPP_BASE_LIBRARY_IMESSAGEQUEUEREPOSITORY_H
#define CPP_BASE_LIBRARY_IMESSAGEQUEUEREPOSITORY_H

#include <memory>
#include <string>
#include <vector>

class MessageQueueEntry;

class IMessageQueueRepository {
public:
    virtual ~IMessageQueueRepository() = default;

    virtual MessageQueueEntry allMessageQueueNameOf(const std::string &name) = 0;

    virtual std::vector<MessageQueueEntry> allOf(const std::string &processName, const std::string &messageQueueName) = 0;

    virtual std::vector<MessageQueueEntry> allProcessNameOf(const std::string &processName) = 0;

    virtual void insertOf(const std::vector<MessageQueueEntry> &entries) = 0;

    virtual void deleteOf(const std::vector<MessageQueueEntry> &entries) = 0;
};

#endif  // CPP_BASE_LIBRARY_IMESSAGEQUEUEREPOSITORY_H
