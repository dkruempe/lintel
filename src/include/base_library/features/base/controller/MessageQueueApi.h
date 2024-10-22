#ifndef CPP_BASE_LIBRARY_MESSAGEQUEUEAPI_H
#define CPP_BASE_LIBRARY_MESSAGEQUEUEAPI_H
#include "MessageQueueDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

class MesssageQueueApi
{
  std::shared_ptr<Client> m_client;

public:
  explicit MesssageQueueApi(const std::shared_ptr<ClientProvider> &clientProvider);

  std::vector<MessageQueueDto> allOf(const std::string &processName, const std::string &messageQueueName);
};

#endif //CPP_BASE_LIBRARY_MESSAGEQUEUEAPI_H