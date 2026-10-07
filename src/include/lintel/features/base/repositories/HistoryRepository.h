#ifndef LINTEL_HISTORYREPOSITORY_H
#define LINTEL_HISTORYREPOSITORY_H

#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/features/base/configuration/DatabaseConnectionEntry.h"
#include "lintel/features/base/models/HistoryEntry.h"

/**
 * Repository for persisting and querying HistoryEntry records in the database.
 */
class HistoryRepository {
private:
    // injections
    std::shared_ptr<DatabaseConnectionConfigurations> m_connectionConfigurations;
    std::shared_ptr<DatabaseConnectionEntry> m_connectionEntry;

public:
    /** @return all history entries */
    [[nodiscard]] std::vector<HistoryEntry> allOf() const;

    /**
     * @param label filter by label
     * @return history entries with the given label
     */
    [[nodiscard]] std::vector<HistoryEntry> allOf(const std::string &label) const;

    /**
     * @param processName filter by process name
     * @param serviceName filter by service name
     * @return history entries matching process and service
     */
    [[nodiscard]] std::vector<HistoryEntry> allOf(const std::string &processName,
                                                   const std::string &serviceName) const;

    /**
     * @param processName filter by process name
     * @param serviceName filter by service name
     * @param label filter by label
     * @return history entries matching all filters
     */
    [[nodiscard]] std::vector<HistoryEntry> allOf(const std::string &processName,
                                                   const std::string &serviceName,
                                                   const std::string &label) const;

    /**
     * @param processName filter by process name
     * @return history entries for the given process
     */
    [[nodiscard]] std::vector<HistoryEntry> allOfProcess(const std::string &processName) const;

    /**
     * @param serviceName filter by service name
     * @return history entries for the given service
     */
    [[nodiscard]] std::vector<HistoryEntry> allOfService(const std::string &serviceName) const;

    /**
     * Insert multiple history entries.
     * @param entries entries to insert
     */
    void insertOf(const std::vector<HistoryEntry> &entries) const;

    /**
     * Delete all entries older than the given timestamp.
     * @param timestamp cutoff timestamp
     */
    void cleanAllOlderThan(date::sys_time<std::chrono::microseconds> timestamp) const;

    /**
     * Constructor.
     * @param databaseConnectionConfigurations database connection configuration
     */
    explicit HistoryRepository(const std::shared_ptr<DatabaseConnectionConfigurations>
                               &databaseConnectionConfigurations);
};

#endif  // LINTEL_HISTORYREPOSITORY_H
