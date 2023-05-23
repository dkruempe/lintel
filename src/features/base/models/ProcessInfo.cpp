#include "base_library/features/base/models/ProcessInfo.h"

ProcessInfo::ProcessInfo(std::shared_ptr<Process> process,
                         boost::process::pid_t id, bool isRunning, int exitCode,
                         std::string groupName, std::string groupId)
        : m_process(std::move(process)),
          m_id(std::move(id)),
          m_isRunning(isRunning),
          m_exitCode(exitCode),
          m_groupName(std::move(groupName)),
          m_groupId(std::move(groupId)) {}

int ProcessInfo::getExitCode() const { return m_exitCode; }

const std::shared_ptr<Process> &ProcessInfo::getProcess() const {
    return m_process;
}

const boost::process::pid_t &ProcessInfo::getProcessId() const { return m_id; }

bool ProcessInfo::isRunning() const { return m_isRunning; }

std::string ProcessInfo::getGroupName() const { return m_groupName; }

std::string ProcessInfo::getGroupId() const { return m_groupId; }