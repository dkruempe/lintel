#include "lintel/features/base/services/MessageQueueService.h"

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/platform/Platform.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/features/base/configuration/MessageQueueComponent.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/msg/MessageQueue.h"
#include "lintel/features/base/repositories/MessageQueueRepository.h"
#include <map>
#include <memory>
#include <unistd.h>
#include <utility>

namespace lintel {
namespace {
  std::string messageQueueKey(const MessageQueueEntry &entry)
  {
    return entry.get_process_name() + "/" + entry.get_message_queue_name();
  }

  std::string getHostName()
  {
    char buf[256];
    if (::gethostname(buf, sizeof(buf)) == 0) { return std::string(buf); }
    return "unknown";
  }
}// namespace

MessageQueueService::MessageQueueService(const std::shared_ptr<Configuration> &configuration,
  std::shared_ptr<ProcessName> processName,
  std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
  std::shared_ptr<SchedulerService> schedulerService)
  : AbstractService<MessageQueueService>(processName->getProcessName()), m_processName(std::move(processName)),
    m_messageQueueRepository(std::move(messageQueueRepository)), m_schedulerService(std::move(schedulerService)),
    m_configurationMap(init(configuration)), m_hostName(getHostName())
{}

std::map<std::string, std::shared_ptr<MessageQueueEntry>> MessageQueueService::init(
  const std::shared_ptr<Configuration> &configuration)
{
  auto confs = configuration->configurationOf<MessageQueueComponent>();
  std::map<std::string, std::shared_ptr<MessageQueueEntry>> map;
  for (const auto &iter : confs) {
    auto messageQueueConf = std::static_pointer_cast<MessageQueueEntry>(iter);
    const std::string index = messageQueueConf->get_message_queue_name();
    map.insert({ index, messageQueueConf });
    LOG_INFO("found message queue {}", messageQueueConf->get_message_queue_name());
  }
  return map;
}

void MessageQueueService::onInitialize()
{
  for (const auto &iter : m_messageQueueRepository->allOf(".*", ".*")) {
    m_messageQueues.insert({ messageQueueKey(iter),
      std::make_shared<MessageQueue<Message>>(
        iter.get_process_name(), iter.get_message_queue_name(), iter.get_max_messages(), m_processName) });
  }
  if (m_schedulerService && !m_configurationMap.empty()) {
    m_schedulerService->schedule_at_fixed_rate(m_heartbeatInterval, m_heartbeatInterval, [this] {
      for (const auto &[name, entry] : m_configurationMap) {
        if (entry->get_process_name() == m_processName->getProcessName()) {
          try {
            updateHeartbeat(name);
          } catch (...) {}
        }
      }
    });
  }
}

void MessageQueueService::onShutdown()
{
  if (m_schedulerService) {
    // best effort - scheduler will be cleaned up
  }
}

void MessageQueueService::registerOwner(const std::string &queueName)
{
  auto found = m_configurationMap.find(queueName);
  if (found == m_configurationMap.end()) { return; }
  if (found->second->get_process_name() != m_processName->getProcessName()) { return; }
  m_messageQueueRepository->registerOwner(queueName,
    m_processName->getProcessName(),
    m_hostName,
    static_cast<std::uint64_t>(currentProcessId()),
    std::chrono::system_clock::now());
}

void MessageQueueService::updateHeartbeat(const std::string &queueName)
{
  m_messageQueueRepository->updateHeartbeat(queueName, std::chrono::system_clock::now());
}

std::size_t MessageQueueService::recoverOrphans(std::chrono::seconds staleThreshold)
{
  auto orphans = m_messageQueueRepository->findOrphans(staleThreshold);
  if (!orphans.empty()) { m_messageQueueRepository->reclaimOrphans(orphans); }
  return orphans.size();
}

std::pair<MessageQueueEntry, int32_t> MessageQueueService::numberMessagesOf(const MessageQueueEntry &entry)
{
  auto found = m_messageQueues.find(messageQueueKey(entry));
  if (found != m_messageQueues.end()) { return { entry, found->second->numberMessagesOf() }; }
  return { entry, MessageQueueCore::numberMessagesOfIfExistsOf(entry.get_message_queue_name()) };
}

std::unique_ptr<MessageQueue<Message>> MessageQueueService::of(const std::string &name)
{
  auto found = m_configurationMap.find(name);
  if (found == m_configurationMap.end()) {
    LOG_ERROR("no message queue found for {}", name);
    throw std::runtime_error("no message queue found for " + name);
  }
  auto entry = found->second;
  registerOwner(name);
  return std::make_unique<MessageQueue<Message>>(
    entry->get_process_name(), entry->get_message_queue_name(), entry->get_max_messages(), m_processName);
}
}// namespace lintel
