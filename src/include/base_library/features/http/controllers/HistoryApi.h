#ifndef HISTORYAPI_H
#define HISTORYAPI_H

#include "base_library/features/base/controller/HistoryDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

class HistoryApi
{
  std::shared_ptr<Client> m_client;

public:
  explicit HistoryApi(const std::shared_ptr<ClientProvider> &clientProvicer);

  std::vector<HistoryDto> allOf(const std::string &processName,
    const std::string &serviceName,
    const std::string &label);
};

#endif //HISTORYAPI_H