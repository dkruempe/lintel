#ifndef CPP_BASE_LIBRARY_PROCESSAPI_H
#define CPP_BASE_LIBRARY_PROCESSAPI_H

#include "base_library/features/base/controller/ProcessGroupDto.h"
#include "base_library/features/base/controller/ProcessInfoDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

class ProcessApi {
private:
    std::shared_ptr<Client> m_client;

public:
    explicit ProcessApi(const std::shared_ptr<ClientProvider> &clientProvicer);

    std::vector<ProcessInfoDto> allOf(const std::string &processName);

    std::vector<ProcessGroupDto> allGroupsOf(const std::string &groupName);

    void startOf(const std::shared_ptr<Process> &process);

    void stopOf(const std::string &id);

    void terminateOf(const std::string &id);
};

#endif  // CPP_BASE_LIBRARY_PROCESSAPI_H
