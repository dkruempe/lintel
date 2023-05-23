#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H

#include <map>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/msg/Message.h"
#include "base_library/features/base/msg/MessageQueue.h"

class MessageQueueService : public AbstractService<MessageQueueService> {
private:
    std::map<std::string, std::shared_ptr<MessageQueue<Message>>> m_messageQueues;
    std::shared_ptr<ProcessName> m_processName;

    static std::map<std::string, std::shared_ptr<MessageQueue<Message>>> init(
            const std::shared_ptr<Configuration> &configuration);

public:
    /**
     * constructor of message queue service
     * @param configuration
     * @param processName
     */
    MessageQueueService(const std::shared_ptr<Configuration> &configuration,
                        std::shared_ptr<ProcessName> processName);

    /**
     * returns message queue to belonging message queue name
     * @param name of message queue
     * @param processName name of process
     * @return message queue itself
     */
    std::shared_ptr<MessageQueue<Message>> of(const std::string &name,
                                              const std::string &processName);

    /**
     * returns self owning message queue, processName is current processName
     * @param name of message queue
     * @return message queue
     */
    std::shared_ptr<MessageQueue<Message>> of(const std::string &name);

    /**
     * returns all available message queues
     * @return message queues
     */
    std::vector<std::shared_ptr<MessageQueue<Message>>> allOf();

    void onInitialize() override;

    void onShutdown() override;
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H
