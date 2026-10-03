#pragma once

#include "base_library/core/utils/ProcessResourceReader.h"
#include "base_library/features/base/models/Process.h"

/**
 * Runtime information about a running process including its PID, status,
 * group membership and (optional) resource usage.
 */
class ProcessInfo
{
private:
  std::shared_ptr<Process> m_process;
  pid_t m_id;
  bool m_isRunning;
  int m_exitCode;
  bool m_exitCodeValid;
  std::string m_groupName;
  std::string m_groupId;
  std::optional<ProcessResourceData> m_resourceData;

public:
  /**
   * Constructor.
   * @param process the process definition
   * @param id OS process ID
   * @param isRunning whether the process is currently running
   * @param exitCode process exit code
   * @param groupName process group name
   * @param groupId process group UUID
   * @param exitCodeValid whether the exit code is meaningful (i.e. the child was waited on)
   * @param resourceData optional resource snapshot
   */
  ProcessInfo(std::shared_ptr<Process> process,
    pid_t id,
    bool isRunning,
    int exitCode,
    std::string groupName,
    std::string groupId,
    bool exitCodeValid = true,
    std::optional<ProcessResourceData> resourceData = std::nullopt);

  /** @return process definition */
  [[nodiscard]] const std::shared_ptr<Process> &getProcess() const;

  /** @return OS process ID */
  [[nodiscard]] pid_t getProcessId() const;

  /** @return true if the process is running */
  [[nodiscard]] constexpr bool isRunning() const { return m_isRunning; }

  /** @return process exit code */
  [[nodiscard]] constexpr int getExitCode() const { return m_exitCode; }

  /** @return true if the reported exit code is meaningful */
  [[nodiscard]] constexpr bool isExitCodeValid() const { return m_exitCodeValid; }

  /** @return group name */
  [[nodiscard]] std::string getGroupName() const;

  /** @return group UUID */
  [[nodiscard]] std::string getGroupId() const;

  /** @return process resource data, if readable on this platform */
  [[nodiscard]] const std::optional<ProcessResourceData> &getResourceData() const;
};