#ifndef CPP_BASE_LIBRARY_PROCESSENTRY_H
#define CPP_BASE_LIBRARY_PROCESSENTRY_H

#include "base_library/features/base/configuration/Entry.h"
#include "base_library/features/base/models/Process.h"
#include "base_library/features/base/models/ProcessGroup.h"

/** Configuration entry for a process or process group */
class ProcessEntry : public Entry {
private:
    /** The process model (if this is a process entry) */
    std::shared_ptr<Process> m_process;
    /** The process group model (if this is a group entry) */
    std::shared_ptr<ProcessGroup> m_processGroup;

public:
    /** Construct a process entry
     * @param component The configuration component name
     * @param process The process model */
    ProcessEntry(std::string_view component, std::shared_ptr<Process> process);

    /** Construct a process group entry
     * @param component The configuration component name
     * @param processGroup The process group model */
    ProcessEntry(std::string_view component,
                 std::shared_ptr<ProcessGroup> processGroup);

    /** Get the process model
     * @return The process shared pointer */
    const std::shared_ptr<Process> &getProcess() const;

    /** Get the process group model
     * @return The process group shared pointer */
    const std::shared_ptr<ProcessGroup> &getProcessGroup() const;
};

#endif  // CPP_BASE_LIBRARY_PROCESSENTRY_H
