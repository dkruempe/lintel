#include "lintel/core/plugins/MessageQueueBootstrapPlugin.h"

#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/core/models/BootstrapSequence.h"
#include "lintel/features/base/configuration/MessageQueueComponent.h"
#include "lintel/features/base/repositories/MessageQueueRepository.h"
#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <utility>

MessageQueueBootstrapPlugin::MessageQueueBootstrapPlugin(
        const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations,
        std::shared_ptr<Configuration> configuration, std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
        std::shared_ptr<ProcessName> processName) :
        m_messageQueueRepository(std::move(messageQueueRepository)),
        m_configuration(std::move(configuration)),
        m_connectionEntry(connectionConfigurations->ofDefault()),
        m_processName(std::move(processName)) {}

BootstrapSequence MessageQueueBootstrapPlugin::getPriority() {
    return BootstrapSequence::MessageQueue;
}

void MessageQueueBootstrapPlugin::onStart() {
    if (m_connectionEntry == nullptr) {
        LOG_INFO("no default database connection - skip message queue bootstrap");
        return;
    }
    auto entries = m_configuration->configurationOf<MessageQueueComponent>();
    std::map<std::string, std::shared_ptr<MessageQueueEntry>> messageQueueEntries;
    for (const auto &entry: entries) {
        const std::shared_ptr<MessageQueueEntry> messageQueueEntry = std::static_pointer_cast<MessageQueueEntry>(entry);
        if (messageQueueEntry->get_process_name() != m_processName->getProcessName()) {
            continue;
        }
        messageQueueEntries.insert({messageQueueEntry->get_message_queue_name(), messageQueueEntry});
    }
    auto tableEntries = m_messageQueueRepository->allProcessNameOf(
            m_processName->getProcessName());
    std::vector<MessageQueueEntry> inserts;
    std::vector<MessageQueueEntry> deletes;
    for (const auto &entry: tableEntries) {
        auto found = messageQueueEntries.find(entry.get_message_queue_name());
        if (found == messageQueueEntries.end()) {
            deletes.push_back(entry);
            messageQueueEntries.erase(entry.get_message_queue_name());
            continue;
        }
        auto tempQueue = found->second;
        if (tempQueue->get_max_messages() != entry.get_max_messages()) {
            deletes.push_back(entry);
            const MessageQueueEntry messageQueueEntry = MessageQueueEntry(*tempQueue);
            inserts.push_back(messageQueueEntry);
            messageQueueEntries.erase(entry.get_message_queue_name());
            continue;
        }
        // existing but equal => just remove from messageQueueEntries
        messageQueueEntries.erase(entry.get_message_queue_name());
    }
    // inserts
    // 1. add missing MessageQueue which are totally new
    for (const auto &item: messageQueueEntries) {
        inserts.emplace_back(*item.second);
    }
    m_messageQueueRepository->deleteOf(deletes);
    m_messageQueueRepository->insertOf(inserts);
}