#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H

#include <map>
#include <base_library/features/base/configuration/MessageQueueEntry.h>

#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/msg/Message.h"
#include "base_library/features/base/msg/MessageQueue.h"

class MessageQueueService : public AbstractService<MessageQueueService> {
private:
    std::shared_ptr<ProcessName> m_processName;
    std::map<std::string, std::shared_ptr<MessageQueueEntry>> m_configurationMap;

    static std::map<std::string, std::shared_ptr<MessageQueueEntry>>
    init(const std::shared_ptr<Configuration> &configuration);

public:
    /**
     * constructor of message queue service
     * @param configuration
     * @param processName
     */
    MessageQueueService(const std::shared_ptr<Configuration> &configuration,
                        std::shared_ptr<ProcessName> processName);

    /**
     * returns self owning message queue, processName is current processName
     * @param name of message queue
     * @return message queue
     */
    std::unique_ptr<MessageQueue<Message>> of(const std::string &name);

    void onInitialize() override {}

    void onShutdown() override {}

};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H
