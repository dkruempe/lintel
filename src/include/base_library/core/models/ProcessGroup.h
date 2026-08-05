#ifndef CPP_BASE_LIBRARY_PROCESSGROUP_H
#define CPP_BASE_LIBRARY_PROCESSGROUP_H

#include <vector>

#include "base_library/core/models/Process.h"

/**
 * A named group of processes.
 */
class ProcessGroup {
private:
    std::string m_id = UUID::generate();
    std::string m_name;
    std::vector<Process> m_processes;

public:
    /**
     * Constructor.
     * @param name group name
     * @param processes processes in this group
     */
    ProcessGroup(std::string name, std::vector<Process> processes);

    /** @return group UUID */
    [[nodiscard]] const std::string &getId() const;

    /** @return processes in this group */
    [[nodiscard]] const std::vector<Process> &getProcesses() const;

    /** @return group name */
    [[nodiscard]] const std::string &getName() const;
};

#endif  // CPP_BASE_LIBRARY_PROCESSGROUP_H
