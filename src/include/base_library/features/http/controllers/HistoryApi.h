#ifndef HISTORYAPI_H
#define HISTORYAPI_H

#include "base_library/features/base/controller/HistoryDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

/** API client for retrieving history data from the HTTP service */
class HistoryApi
{
  std::shared_ptr<Client> m_client;

public:
  explicit HistoryApi(const std::shared_ptr<ClientProvider> &clientProvicer);

  /**
   * @param processName process name filter
   * @param serviceName service name filter
   * @param label label filter
   * @return list of matching history DTOs
   */
  std::vector<HistoryDto> allOf(const std::string &processName,
    const std::string &serviceName,
    const std::string &label);
};

#endif //HISTORYAPI_H
