#include "base_library/features/base/services/MessageQueueService.h"

#include "base_library/features/base/configuration/MessageQueueComponent.h"
#include "base_library/features/base/configuration/MessageQueueEntry.h"

MessageQueueService::MessageQueueService(
        const std::shared_ptr<Configuration> &configuration,
        std::shared_ptr<ProcessName> processName)
        : AbstractService<MessageQueueService>(processName->getProcessName()),
          m_messageQueues(init(configuration)),
          m_processName(std::move(processName)) {
    // I Validation checks
    // 1. for given m_processName exists an entry in the Process definition ?!
    //    -> no exception abort startup
    // 2. size change => how to detect current size in case of still existing queue
}

std::map<std::string, std::shared_ptr<MessageQueue<Message>>>
MessageQueueService::init(const std::shared_ptr<Configuration> &configuration) {
    auto confs = configuration->configurationOf<MessageQueueComponent>();
    std::map<std::string, std::shared_ptr<MessageQueue<Message>>> map;
    for (const auto &iter: confs) {
        auto messageQueueConf = std::static_pointer_cast<MessageQueueEntry>(iter);
        const std::string index = messageQueueConf->get_process_name() + "_" +
                                  messageQueueConf->get_message_queue_name();
        map.insert({index, std::make_shared<MessageQueue<Message>>(
                messageQueueConf->get_process_name(),
                messageQueueConf->get_message_queue_name(),
                messageQueueConf->get_max_messages(),
                messageQueueConf->get_remove_on_shutdown())});
    }
    return map;
}

std::vector<std::shared_ptr<MessageQueue<Message>>>
MessageQueueService::allOf() {
    std::vector<std::shared_ptr<MessageQueue<Message>>> vec;
    std::transform(
            m_messageQueues.begin(), m_messageQueues.end(), std::back_inserter(vec),
            [](const std::pair<std::string, std::shared_ptr<MessageQueue<Message>>> p)
                    -> std::shared_ptr<MessageQueue<Message>> { return p.second; });
    return vec;
}

std::shared_ptr<MessageQueue<Message>> MessageQueueService::of(
        const std::string &name, const std::string &processName) {
    auto found = m_messageQueues.find(processName + "_" + name);
    if (found == m_messageQueues.end()) {
        return nullptr;
    }
    return found->second;
}

std::shared_ptr<MessageQueue<Message>> MessageQueueService::of(
        const std::string &name) {
    return of(name, m_processName->getProcessName());
}

void MessageQueueService::onInitialize() {}

void MessageQueueService::onShutdown() {
    for (const auto &[name, queue]: m_messageQueues) {
        if (queue->getProcessName() != m_processName->getProcessName()) {
            continue;
        }
        if (!queue->isRemoveMessageQueueAfterShutdown()) {
            continue;
        }
        queue->removeOf(queue->getName());
    }
}