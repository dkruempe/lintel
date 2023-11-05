#include "base_library/core/services/AbstractService.h"
#include "base_library/features/base/models/HistoryEntry.h"
#include "base_library/features/base/repositories/HistoryRepository.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/MessageQueueService.h"

#include <ctime>

#ifndef CPP_BASE_LIBRARY_HISTORYSERVICE_H
#define CPP_BASE_LIBRARY_HISTORYSERVICE_H

class HistoryService : public AbstractService<HistoryService> {
private:
    // properties
    DEFINE_PROPERTY(m_duration, std::chrono::seconds, std::chrono::seconds(1),
                    "Duration of scheduling", true);
    DEFINE_PROPERTY(m_period, std::chrono::seconds, std::chrono::seconds(1),
                    "Period of scheduling", true);
    DEFINE_PROPERTY(m_forceQueue, bool, false, "Force usage of queue", true);
    DEFINE_PROPERTY(m_commitRate, std::size_t, 100, "Commit Rate", true);
    // injections
    std::shared_ptr<SchedulerService> m_schedulerService;
    std::shared_ptr<HistoryRepository> m_historyRepository;
    std::shared_ptr<MessageQueueService> m_messageQueueSerivice;
    std::shared_ptr<ProcessName> m_processName;
    // variables
    std::vector<HistoryEntry> m_historyEntries;
    std::mutex m_historyEntriesMutex;
    std::atomic_bool m_running = true;
    std::unique_ptr<MessageQueue<Message>> m_historyMessageQueue;
    std::unique_ptr<MessageQueue<Message>> m_historyMessageQueueReceive;

    void run();

    struct HistoryMessage {
        char processName[100];
        char serviceName[100];
        char label[100];
        char text[300];
        std::time_t createdTimestamp;
    };

public:
    HistoryService(const std::shared_ptr<ProcessName> &processName,
                   std::shared_ptr<SchedulerService> schedulerService,
                   std::shared_ptr<HistoryRepository> historyRepository,
                   std::shared_ptr<MessageQueueService> messageQueueService);

    ~HistoryService() override;

    void onInitialize() override;

    void onShutdown() override;

    void historizeOf(std::vector<HistoryEntry> entries);

    [[nodiscard]] std::vector<HistoryEntry> allOf() const;

    [[nodiscard]] std::vector<HistoryEntry> allOf(const std::string &label) const;

    [[nodiscard]] std::vector<HistoryEntry> allOf(
            const std::string &processName, const std::string &serviceName) const;

    [[nodiscard]] std::vector<HistoryEntry> allOfProcess(
            const std::string &processName) const;

    [[nodiscard]] std::vector<HistoryEntry> allOfService(
            const std::string &serviceName) const;
};

#endif  // CPP_BASE_LIBRARY_HISTORYSERVICE_H
