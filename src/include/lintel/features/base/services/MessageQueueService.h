#ifndef LINTEL_MESSAGEQUEUESERVICE_H
#define LINTEL_MESSAGEQUEUESERVICE_H

#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/repositories/IMessageQueueRepository.h"
#include <chrono>
#include <map>
#include <memory>
#include <string>

#include "lintel/core/configuration/Configuration.h"
#include "lintel/core/models/ProcessName.h"
#include "lintel/core/services/AbstractService.h"
#include "lintel/features/base/msg/Message.h"
#include "lintel/features/base/msg/MessageQueue.h"
#include "lintel/features/base/services/SchedulerService.h"
#include <atomic>

namespace lintel {
/**
 * Service that manages creation, retrieval, and lifecycle of MessageQueue instances.
 */
class MessageQueueService : public AbstractService<MessageQueueService>
{
private:
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<IMessageQueueRepository> m_messageQueueRepository;
  std::shared_ptr<SchedulerService> m_schedulerService;
  std::map<std::string, std::shared_ptr<MessageQueueEntry>> m_configurationMap;
  std::map<std::string, std::shared_ptr<MessageQueue<Message>>> m_messageQueues{};
  std::string m_hostName;
  std::chrono::milliseconds m_heartbeatInterval{ 1000 };
  /** False once onShutdown() ran; the heartbeat task captures it to stop. */
  std::atomic<bool> m_running{ false };

  static std::map<std::string, std::shared_ptr<MessageQueueEntry>> init(
    const std::shared_ptr<Configuration> &configuration);

public:
  /**
   * constructor of message queue service
   * @param configuration parsed bootstrap configuration
   * @param processName name of the current process
   * @param messageQueueRepository repository holding the queue metadata
   * @param schedulerService scheduler that runs the queue heartbeat (may be null)
   */
  MessageQueueService(const std::shared_ptr<Configuration> &configuration,
    std::shared_ptr<ProcessName> processName,
    std::shared_ptr<IMessageQueueRepository> messageQueueRepository,
    std::shared_ptr<SchedulerService> schedulerService = nullptr);

  /**
   * Returns a self-owned message queue (processName matches current process).
   * @param name of message queue
   * @return message queue instance
   */
  std::unique_ptr<MessageQueue<Message>> of(const std::string &name);

  /**
   * Get the number of messages for a given queue entry.
   * @param entry queue entry
   * @return pair of entry and message count
   */
  std::pair<MessageQueueEntry, int32_t> numberMessagesOf(const MessageQueueEntry &entry);

  void registerOwner(const std::string &queueName);
  void updateHeartbeat(const std::string &queueName);
  std::size_t recoverOrphans(std::chrono::seconds staleThreshold = std::chrono::seconds{ 60 });

  void onInitialize() override;

  void onShutdown() override;
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUESERVICE_H