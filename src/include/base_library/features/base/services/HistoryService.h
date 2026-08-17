#include "base_library/core/services/PropertyRegistration.h"
#include "base_library/features/base/models/HistoryEntry.h"
#include "base_library/features/base/repositories/HistoryRepository.h"
#include "base_library/features/base/services/IHistoryService.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/MessageQueueService.h"

#include <ctime>
#include <memory>

#ifndef CPP_BASE_LIBRARY_HISTORYSERVICE_H
#define CPP_BASE_LIBRARY_HISTORYSERVICE_H

/**
 * Service for recording, queuing, and querying history entries.
 * Uses a message queue for inter-process history submission.
 */
class HistoryService : public PropertyRegistration<HistoryService>,
                       public IHistoryService,
                       public std::enable_shared_from_this<HistoryService> {
private:
    // properties
    std::shared_ptr<Property<std::chrono::seconds>> m_duration;
    std::shared_ptr<Property<std::chrono::seconds>> m_period;
    std::shared_ptr<Property<bool>> m_forceQueue;
    std::shared_ptr<Property<std::size_t>> m_commitRate;
    // injections
    std::shared_ptr<SchedulerService> m_schedulerService;
    std::shared_ptr<HistoryRepository> m_historyRepository;
    std::shared_ptr<MessageQueueService> m_messageQueueSerivice;
    std::string m_ownProcessName;
    std::shared_ptr<ProcessName> m_processName;
    // variables
    std::vector<HistoryEntry> m_historyEntries;
    std::mutex m_historyEntriesMutex;
    std::atomic_bool m_running = true;
    std::unique_ptr<MessageQueue<Message>> m_historyMessageQueue;
    std::unique_ptr<MessageQueue<Message>> m_historyMessageQueueReceive;

    /** Background worker that processes queued history entries. */
    void run();

    /** Message struct for transferring history entries via message queue. */
    struct HistoryMessage {
        char processName[100];
        char serviceName[100];
        char label[100];
        char text[300];
        std::time_t createdTimestamp;
        int64_t createdTimestampMics;
    };

public:
    /**
     * Constructor.
     * @param processName current process name
     * @param schedulerService scheduler for periodic commits
     * @param historyRepository persistence repository
     * @param messageQueueService message queue for inter-process communication
     */
    HistoryService(std::shared_ptr<ProcessName> processName,
                   std::shared_ptr<SchedulerService> schedulerService,
                   std::shared_ptr<HistoryRepository> historyRepository,
                   std::shared_ptr<MessageQueueService> messageQueueService);

    ~HistoryService() override;

    void onInitialize() override;

    void onShutdown() override;

    /**
     * Record history entries (may be queued for batch insert).
     * @param entries entries to record
     */
    void historizeOf(std::vector<HistoryEntry> entries) override;

    /** @return the process name of the owning process */
    [[nodiscard]] const std::string &getProcessName() const override;

    /** @return all history entries */
    [[nodiscard]] std::vector<HistoryEntry> allOf() const override;

    /**
     * @param label filter by label
     * @return matching entries
     */
    [[nodiscard]] std::vector<HistoryEntry> allOf(const std::string &label) const override;

    /**
     * @param processName filter by process name
     * @param serviceName filter by service name
     * @return matching entries
     */
    [[nodiscard]] std::vector<HistoryEntry> allOf(
            const std::string &processName,
            const std::string &serviceName) const override;

    /**
     * @param processName filter by process name
     * @return matching entries
     */
    [[nodiscard]] std::vector<HistoryEntry> allOfProcess(
            const std::string &processName) const override;

    /**
     * @param serviceName filter by service name
     * @return matching entries
     */
    [[nodiscard]] std::vector<HistoryEntry> allOfService(
            const std::string &serviceName) const override;
};

#endif  // CPP_BASE_LIBRARY_HISTORYSERVICE_H
