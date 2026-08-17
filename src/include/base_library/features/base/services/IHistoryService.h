#ifndef CPP_BASE_LIBRARY_IHISTORYSERVICE_H
#define CPP_BASE_LIBRARY_IHISTORYSERVICE_H

#include <string>
#include <vector>

#include "base_library/features/base/models/HistoryEntry.h"

/**
 * Interface for the history service.
 *
 * The real implementation (HistoryService) is only created in the process
 * that the configuration explicitly declares as its owner. Every other
 * process receives a NoopHistoryService so that services depending on this
 * interface stay resolvable without ever running a history service.
 */
class IHistoryService {
public:
    virtual ~IHistoryService() = default;

    /**
     * Record history entries (may be queued for batch insert).
     * @param entries entries to record
     */
    virtual void historizeOf(std::vector<HistoryEntry> entries) = 0;

    /** @return the process name of the owning process */
    [[nodiscard]] virtual const std::string &getProcessName() const = 0;

    /** @return all history entries */
    [[nodiscard]] virtual std::vector<HistoryEntry> allOf() const = 0;

    /**
     * @param label filter by label
     * @return matching entries
     */
    [[nodiscard]] virtual std::vector<HistoryEntry> allOf(
            const std::string &label) const = 0;

    /**
     * @param processName filter by process name
     * @param serviceName filter by service name
     * @return matching entries
     */
    [[nodiscard]] virtual std::vector<HistoryEntry> allOf(
            const std::string &processName,
            const std::string &serviceName) const = 0;

    /**
     * @param processName filter by process name
     * @return matching entries
     */
    [[nodiscard]] virtual std::vector<HistoryEntry> allOfProcess(
            const std::string &processName) const = 0;

    /**
     * @param serviceName filter by service name
     * @return matching entries
     */
    [[nodiscard]] virtual std::vector<HistoryEntry> allOfService(
            const std::string &serviceName) const = 0;
};

#endif  // CPP_BASE_LIBRARY_IHISTORYSERVICE_H
