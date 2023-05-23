#include "base_library/features/base/models/ProcessGroup.h"

ProcessGroup::ProcessGroup(std::string name, std::vector<Process> processes)
        : m_name(std::move(name)), m_processes(std::move(processes)) {}

const std::string &ProcessGroup::getId() const { return m_id; }

const std::vector<Process> &ProcessGroup::getProcesses() const {
    return m_processes;
}

const std::string &ProcessGroup::getName() const { return m_name; }
