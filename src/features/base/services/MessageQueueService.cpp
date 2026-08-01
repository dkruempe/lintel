#include "base_library/features/base/services/MessageQueueService.h"

#include "base_library/features/base/configuration/MessageQueueComponent.h"
#include "base_library/features/base/configuration/MessageQueueEntry.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/msg/MessageQueue.h"
#include "base_library/features/base/repositories/MessageQueueRepository.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/configuration/Configuration.h"
#include <map>
#include <memory>
#include <utility>

MessageQueueService::MessageQueueService(
  const std::shared_ptr<Configuration> &configuration,
  std::shared_ptr<ProcessName> processName,
  std::shared_ptr<IMessageQueueRepository> messageQueueRepository)
  : AbstractService<MessageQueueService>(processName->getProcessName()),
    m_processName(std::move(processName)),
    m_messageQueueRepository(std::move(messageQueueRepository)),
    m_configurationMap(init(configuration))
{
}

std::map<std::string, std::shared_ptr<MessageQueueEntry> >
  MessageQueueService::init(const std::shared_ptr<Configuration> &configuration)
{
  auto confs = configuration->configurationOf<MessageQueueComponent>();
  std::map<std::string, std::shared_ptr<MessageQueueEntry> > map;
  for (const auto &iter : confs) {
    auto messageQueueConf = std::static_pointer_cast<MessageQueueEntry>(iter);
    const std::string index = messageQueueConf->get_message_queue_name();
    map.insert({ index, messageQueueConf });
    LOG_INFO("found message queue {}", messageQueueConf->get_message_queue_name());
  }
  return map;
}

void MessageQueueService::onInitialize() {
  for (const auto &iter : m_messageQueueRepository->allOf(".*", ".*")) {
    m_messageQueues.insert({ iter.get_message_queue_name(),
                             std::make_shared<MessageQueue<Message> >(iter.get_process_name(),
                               iter.get_message_queue_name(),
                               iter.get_max_messages(),
                               m_processName) });
  }
}

std::pair<MessageQueueEntry, int32_t> MessageQueueService::numberMessagesOf(const MessageQueueEntry &entry)
{
  auto found = m_messageQueues.find(entry.get_message_queue_name());
  if (found != m_messageQueues.end()) { return { entry, found->second->numberMessagesOf() }; }
  auto result = std::make_unique<MessageQueue<Message> >(entry.get_process_name(),
    entry.get_message_queue_name(),
    entry.get_max_messages(),
    m_processName);
  return { entry, result->numberMessagesOf() };
}

std::unique_ptr<MessageQueue<Message> > MessageQueueService::of(
  const std::string &name)
{

  auto found = m_configurationMap.find(name);
  if (found == m_configurationMap.end()) {
    LOG_ERROR("no message queue found for {}", name);
    throw std::runtime_error("no message queue found for " + name);
  }
  auto entry = found->second;
  return std::make_unique<MessageQueue<Message> >(entry->get_process_name(),
    entry->get_message_queue_name(),
    entry->get_max_messages(),
    m_processName);
}