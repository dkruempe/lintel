#ifndef CPP_BASE_LIBRARY_NOOPHISTORYSERVICE_H
#define CPP_BASE_LIBRARY_NOOPHISTORYSERVICE_H

#include <memory>
#include <string>
#include <vector>

#include "base_library/features/base/models/HistoryEntry.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/services/IHistoryService.h"

/**
 * History service implementation that does nothing.
 *
 * Used in processes that are not the configured owner of the history
 * service, so that the IHistoryService is always resolvable while the real
 * HistoryService is only ever created in the owner process.
 */
class NoopHistoryService : public IHistoryService {
private:
    std::string m_processName;

public:
    /** Construct a NoopHistoryService.
     * @param processName the process name of the running process */
    explicit NoopHistoryService(std::shared_ptr<ProcessName> processName)
            : m_processName(processName->getProcessName()) {}

    void historizeOf(std::vector<HistoryEntry> /*entries*/) override {}

    [[nodiscard]] const std::string &getProcessName() const override {
        return m_processName;
    }

    [[nodiscard]] std::vector<HistoryEntry> allOf() const override {
        return {};
    }

    [[nodiscard]] std::vector<HistoryEntry> allOf(
            const std::string & /*label*/) const override {
        return {};
    }

    [[nodiscard]] std::vector<HistoryEntry> allOf(
            const std::string & /*processName*/,
            const std::string & /*serviceName*/) const override {
        return {};
    }

    [[nodiscard]] std::vector<HistoryEntry> allOfProcess(
            const std::string & /*processName*/) const override {
        return {};
    }

    [[nodiscard]] std::vector<HistoryEntry> allOfService(
            const std::string & /*serviceName*/) const override {
        return {};
    }
};

#endif  // CPP_BASE_LIBRARY_NOOPHISTORYSERVICE_H
