#ifndef LINTEL_MESSAGEQUEUEAPI_H
#define LINTEL_MESSAGEQUEUEAPI_H
#include "lintel/features/base/controller/MessageQueueDto.h"
#include "lintel/features/base/repositories/MessageQueueQuery.h"
#include "lintel/features/http/provider/ClientProvider.h"

#include <cstddef>
#include <optional>

namespace lintel {
/** API client for retrieving message queue data from the HTTP service */
class MesssageQueueApi
{
  std::shared_ptr<Client> m_client;

public:
  explicit MesssageQueueApi(const std::shared_ptr<ClientProvider> &clientProvider);

  /**
   * @param filter process/queue name patterns
   * @return list of matching message queue DTOs
   */
  std::vector<MessageQueueDto> allOf(const MessageQueueFilter &filter);

  /**
   * Remove all messages currently queued in the given queue.
   * @param messageQueueName exact queue name
   * @return number of removed messages, nullopt if the purge was rejected
   */
  std::optional<std::int32_t> purgeOf(const std::string &messageQueueName);

  /**
   * Reclaim the queue entries whose owner process is gone.
   * @return number of reclaimed entries, nullopt if the request was rejected
   */
  std::optional<std::size_t> recoverOf();
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEAPI_H
