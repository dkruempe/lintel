#pragma once

#include "base_library/features/base/models/Process.h"

#include <boost/process/v1/child.hpp>

class ProcessInfo {
private:
    std::shared_ptr<Process> m_process;
    boost::process::v1::pid_t m_id;
    bool m_isRunning;
    int m_exitCode;
    std::string m_groupName;
    std::string m_groupId;

public:
    ProcessInfo(std::shared_ptr<Process> process, boost::process::v1::pid_t id,
                bool isRunning, int exitCode, std::string groupName,
                std::string groupId);

    [[nodiscard]] const std::shared_ptr<Process> &getProcess() const;

    [[nodiscard]] const boost::process::v1::pid_t &getProcessId() const;

    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] int getExitCode() const;

    [[nodiscard]] std::string getGroupName() const;

    [[nodiscard]] std::string getGroupId() const;
};