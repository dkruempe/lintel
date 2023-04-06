#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H

#include <base_library/features/base/models/ProcessName.h>

#include <map>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/msg/Message.h"
#include "base_library/features/base/msg/MessageQueue.h"

class MessageQueueService {
 private:
  std::map<std::string, MessageQueue<Message>> m_messageQueues;
  std::shared_ptr<ProcessName> m_processName;

 public:
  /**
   * constructor of message queue service
   * @param configuration
   * @param processName
   */
  MessageQueueService(std::shared_ptr<Configuration> configuration,
                      std::shared_ptr<ProcessName> processName);

  /**
   * returns message queue to belonging message queue name
   * @param name of message queue
   * @return message queue itself
   */
  MessageQueue<Message> of(const std::string &name);

  /**
   * returns self owning message queue
   * @return message queue
   */
  MessageQueue<Message> of();

  /**
   * returns all available message queues
   * @return message queues
   */
  std::vector<MessageQueue<Message>> allOf();
};

#endif  // CPP_BASE_LIBRARY_MESSAGEQUEUESERVICE_H
