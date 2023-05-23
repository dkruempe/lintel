#ifndef CPP_BASE_LIBRARY_PROCESSENTRY_H
#define CPP_BASE_LIBRARY_PROCESSENTRY_H

#include "base_library/features/base/configuration/Entry.h"
#include "base_library/features/base/models/Process.h"
#include "base_library/features/base/models/ProcessGroup.h"

class ProcessEntry : public Entry {
private:
    std::shared_ptr<Process> m_process;
    std::shared_ptr<ProcessGroup> m_processGroup;

public:
    ProcessEntry(std::string_view component, std::shared_ptr<Process> process);

    ProcessEntry(std::string_view component,
                 std::shared_ptr<ProcessGroup> processGroup);

    const std::shared_ptr<Process> &getProcess() const;

    const std::shared_ptr<ProcessGroup> &getProcessGroup() const;
};

#endif  // CPP_BASE_LIBRARY_PROCESSENTRY_H
