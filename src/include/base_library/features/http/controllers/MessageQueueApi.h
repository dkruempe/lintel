#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEAPI_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEAPI_H
#include "base_library/features/base/controller/MessageQueueDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

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
};

#endif //CPP_BASE_LIBRARY_MESSAGEQUEUEAPI_H
