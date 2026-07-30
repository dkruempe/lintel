#pragma once

#include "base_library/features/base/models/Process.h"

#include <boost/process/v1/child.hpp>

/**
 * Runtime information about a running process including its PID, status, and group membership.
 */
class ProcessInfo {
private:
    std::shared_ptr<Process> m_process;
    boost::process::v1::pid_t m_id;
    bool m_isRunning;
    int m_exitCode;
    std::string m_groupName;
    std::string m_groupId;

public:
    /**
     * Constructor.
     * @param process the process definition
     * @param id OS process ID
     * @param isRunning whether the process is currently running
     * @param exitCode process exit code
     * @param groupName process group name
     * @param groupId process group UUID
     */
    ProcessInfo(std::shared_ptr<Process> process, boost::process::v1::pid_t id,
                bool isRunning, int exitCode, std::string groupName,
                std::string groupId);

    /** @return process definition */
    [[nodiscard]] const std::shared_ptr<Process> &getProcess() const;

    /** @return OS process ID */
    [[nodiscard]] const boost::process::v1::pid_t &getProcessId() const;

    /** @return true if the process is running */
    [[nodiscard]] bool isRunning() const;

    /** @return process exit code */
    [[nodiscard]] int getExitCode() const;

    /** @return group name */
    [[nodiscard]] std::string getGroupName() const;

    /** @return group UUID */
    [[nodiscard]] std::string getGroupId() const;
};