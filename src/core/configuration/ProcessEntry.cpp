#include "base_library/core/configuration/ProcessEntry.h"

ProcessEntry::ProcessEntry(std::string_view component,
                           std::shared_ptr<Process> process)
        : Entry(component),
          m_process(std::move(process)),
          m_processGroup(nullptr) {}

ProcessEntry::ProcessEntry(std::string_view component,
                           std::shared_ptr<ProcessGroup> processGroup)
        : Entry(component),
          m_process(nullptr),
          m_processGroup(std::move(processGroup)) {}

const std::shared_ptr<Process> &ProcessEntry::getProcess() const {
    return m_process;
}

const std::shared_ptr<ProcessGroup> &ProcessEntry::getProcessGroup() const {
    return m_processGroup;
}
