#include "base_library/features/base/models/ProcessInfo.h"

ProcessInfo::ProcessInfo(std::shared_ptr<Process> process,
  boost::process::v1::pid_t id,
  bool isRunning,
  int exitCode,
  std::string groupName,
  std::string groupId,
  bool exitCodeValid,
  std::optional<ProcessResourceData> resourceData)
  : m_process(std::move(process)), m_id(id), m_isRunning(isRunning), m_exitCode(exitCode),
    m_exitCodeValid(exitCodeValid), m_groupName(std::move(groupName)), m_groupId(std::move(groupId)),
    m_resourceData(std::move(resourceData))
{}

const std::shared_ptr<Process> &ProcessInfo::getProcess() const { return m_process; }

const boost::process::v1::pid_t &ProcessInfo::getProcessId() const { return m_id; }

std::string ProcessInfo::getGroupName() const { return m_groupName; }

std::string ProcessInfo::getGroupId() const { return m_groupId; }

const std::optional<ProcessResourceData> &ProcessInfo::getResourceData() const { return m_resourceData; }
