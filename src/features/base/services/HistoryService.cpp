#include "base_library/features/base/services/HistoryService.h"

HistoryService::HistoryService(
        const std::shared_ptr<ProcessName> &processName,
        std::shared_ptr<SchedulerService> schedulerService,
        std::shared_ptr<HistoryRepository> historyRepository)
        : AbstractService<HistoryService>(processName->getProcessName()),
          m_schedulerService(std::move(schedulerService)),
          m_historyRepository(std::move(historyRepository)) {}

void HistoryService::onInitialize() {
    m_schedulerService->schedule_at_fixed_rate(
            m_duration->getValue(), m_period->getValue(), [&]() { run(); });
}

void HistoryService::run() {
    std::vector<HistoryEntry> temp{};
    {
        std::lock_guard<std::mutex> locker(m_historyEntriesMutex);
        temp = m_historyEntries;
        m_historyEntries.clear();
    }
    m_historyRepository->insertOf(temp);
}

void HistoryService::historizeOf(std::vector<HistoryEntry> entries) {
    std::lock_guard<std::mutex> locker(m_historyEntriesMutex);
    m_historyEntries.insert(m_historyEntries.end(), entries.begin(),
                            entries.end());
}

std::vector<HistoryEntry> HistoryService::allOf() const {
    return m_historyRepository->allOf();
}

std::vector<HistoryEntry> HistoryService::allOf(
        const std::string &label) const {
    return m_historyRepository->allOf(label);
}

std::vector<HistoryEntry> HistoryService::allOf(
        const std::string &processName, const std::string &serviceName) const {
    return m_historyRepository->allOf(processName, serviceName);
}

std::vector<HistoryEntry> HistoryService::allOfProcess(
        const std::string &processName) const {
    return m_historyRepository->allOfProcess(processName);
}

std::vector<HistoryEntry> HistoryService::allOfService(
        const std::string &serviceName) const {
    return m_historyRepository->allOfService(serviceName);
}

HistoryService::~HistoryService() {
    // make sure that all available HistoryEntries are persisted
    run();
}
