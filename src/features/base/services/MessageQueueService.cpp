#include "base_library/features/base/services/MessageQueueService.h"

#include "base_library/features/base/configuration/MessageQueueComponent.h"
#include "base_library/features/base/configuration/MessageQueueEntry.h"
#include <regex>
#include <memory>

MessageQueueService::MessageQueueService(
        const std::shared_ptr<Configuration> &configuration,
        std::shared_ptr<ProcessName> processName)
        : AbstractService<MessageQueueService>(processName->getProcessName()),
          m_processName(std::move(processName)),
          m_configurationMap(init(configuration)){
    // I Validation checks
    // 1. for given m_processName exists an entry in the Process definition ?!
    //    -> no exception abort startup
    // 2. size change => how to detect current size in case of still existing queue
}

std::map<std::string, std::shared_ptr<MessageQueueEntry>>
MessageQueueService::init(const std::shared_ptr<Configuration> &configuration) {
    auto confs = configuration->configurationOf<MessageQueueComponent>();
    std::map<std::string, std::shared_ptr<MessageQueueEntry>> map;
    for (const auto &iter: confs) {
        auto messageQueueConf = std::static_pointer_cast<MessageQueueEntry>(iter);
        const std::string index = messageQueueConf->get_message_queue_name();
        map.insert({index, messageQueueConf});
        LOG_INFO("found message queue {}", messageQueueConf->get_message_queue_name());
    }
    return map;
}

std::unique_ptr<MessageQueue<Message>> MessageQueueService::of(
        const std::string &name) {

    auto found = m_configurationMap.find(name);
    if (found == m_configurationMap.end()) {
        LOG_ERROR("no message queue found for {}", name);
        throw std::runtime_error("no message queue found for " + name);
    }
    auto entry = found->second;
    return std::make_unique<MessageQueue<Message>>(entry->get_process_name(), entry->get_message_queue_name(),
                                                   entry->get_max_messages(), m_processName);
}