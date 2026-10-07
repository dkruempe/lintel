#ifndef LINTEL_MESSAGEQUEUESERVICE_H
#define LINTEL_MESSAGEQUEUESERVICE_H

#include <map>
#include <string>
#include <memory>
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/repositories/IMessageQueueRepository.h"

#include "lintel/core/services/AbstractService.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/msg/Message.h"
#include "lintel/features/base/msg/MessageQueue.h"

/**
 * Service that manages creation, retrieval, and lifecycle of MessageQueue instances.
 */
class MessageQueueService : public AbstractService<MessageQueueService>
{
private:
  std::shared_ptr<ProcessName> m_processName;
  std::shared_ptr<IMessageQueueRepository> m_messageQueueRepository;
  std::map<std::string, std::shared_ptr<MessageQueueEntry> > m_configurationMap;
  std::map<std::string, std::shared_ptr<MessageQueue<Message> > > m_messageQueues{};

  static std::map<std::string, std::shared_ptr<MessageQueueEntry> >
    init(const std::shared_ptr<Configuration> &configuration);

public:
  /**
   * constructor of message queue service
   * @param configuration parsed bootstrap configuration
   * @param processName name of the current process
   * @param messageQueueRepository repository holding the queue metadata
   */
  MessageQueueService(const std::shared_ptr<Configuration> &configuration,
    std::shared_ptr<ProcessName> processName,
    std::shared_ptr<IMessageQueueRepository> messageQueueRepository);

  /**
   * Returns a self-owned message queue (processName matches current process).
   * @param name of message queue
   * @return message queue instance
   */
  std::unique_ptr<MessageQueue<Message> > of(const std::string &name);

  /**
   * Get the number of messages for a given queue entry.
   * @param entry queue entry
   * @return pair of entry and message count
   */
  std::pair<MessageQueueEntry, int32_t> numberMessagesOf(const MessageQueueEntry &entry);

  void onInitialize() override;

  void onShutdown() override {}

};

#endif  // LINTEL_MESSAGEQUEUESERVICE_H