#ifndef CPP_BASE_LIBRARY_PROCESSAPI_H
#define CPP_BASE_LIBRARY_PROCESSAPI_H

#include "base_library/features/base/controller/ProcessGroupDto.h"
#include "base_library/features/base/controller/ProcessInfoDto.h"
#include "base_library/features/http/provider/ClientProvider.h"

/** API client for process management via HTTP */
class ProcessApi
{
private:
  std::shared_ptr<Client> m_client;

public:
  explicit ProcessApi(const std::shared_ptr<ClientProvider> &clientProvicer);

  /** @param processName name filter; return all matching processes */
  std::vector<ProcessInfoDto> allOf(const std::string &processName);

  /** @param groupName name filter; return all matching process groups */
  std::vector<ProcessGroupDto> allGroupsOf(const std::string &groupName);

  /** Start a process */
  void startOf(const std::shared_ptr<Process> &process);

  /** Stop a process by ID */
  void stopOf(const std::string &id);

  /** Terminate a process by ID */
  void terminateOf(const std::string &id);

  /** Restart a process by ID */
  void restartOf(const std::string &id);

  /** Reset the restart counter and failure state of a process by ID */
  void resetOf(const std::string &id);

  /** Query the resource/health snapshot of a process by ID
   * @param id the process ID
   * @return the process info DTO (with resource data), or nullopt if unavailable */
  std::optional<ProcessInfoDto> healthOf(const std::string &id);
};

#endif// CPP_BASE_LIBRARY_PROCESSAPI_H
