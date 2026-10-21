#ifndef LINTEL_MESSAGEQUEUEAPI_H
#define LINTEL_MESSAGEQUEUEAPI_H
#include "lintel/features/base/controller/MessageQueueDto.h"
#include "lintel/features/http/provider/ClientProvider.h"

#include <optional>

namespace lintel {
/** API client for retrieving message queue data from the HTTP service */
class MesssageQueueApi
{
  std::shared_ptr<Client> m_client;

public:
  explicit MesssageQueueApi(const std::shared_ptr<ClientProvider> &clientProvider);

  /**
   * @param processName process name filter
   * @param messageQueueName message queue name filter
   * @return list of matching message queue DTOs
   */
  std::vector<MessageQueueDto> allOf(const std::string &processName, const std::string &messageQueueName);

  /**
   * Remove all messages currently queued in the given queue.
   * @param messageQueueName exact queue name
   * @return number of removed messages, nullopt if the purge was rejected
   */
  std::optional<std::int32_t> purgeOf(const std::string &messageQueueName);
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEAPI_H
