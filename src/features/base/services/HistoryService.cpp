#include "base_library/features/base/services/HistoryService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/features/base/msg/MessageQueues.h"

#include <algorithm>

HistoryService::HistoryService(
        std::shared_ptr<ProcessName> processName,
        std::shared_ptr<SchedulerService> schedulerService,
        std::shared_ptr<HistoryRepository> historyRepository,
        std::shared_ptr<MessageQueueService> messageQueueService)
        : PropertyRegistration(processName->getProcessName()),
          m_schedulerService(std::move(schedulerService)),
          m_historyRepository(std::move(historyRepository)), m_messageQueueSerivice(std::move(messageQueueService)),
          m_processName(std::move(processName)), m_historyMessageQueue(
                m_messageQueueSerivice->of(static_cast<MessageQueues>(MessageQueues::HISTORY).getMessageQueueName())),
          m_historyMessageQueueReceive(m_messageQueueSerivice->of(
                  static_cast<MessageQueues>(MessageQueues::HISTORY).getMessageQueueName())) {
    m_duration = registerProperty<std::chrono::seconds>(
            "m_duration", std::chrono::seconds(1),
            "Duration of scheduling", true, __FILE__, __LINE__);
    m_period = registerProperty<std::chrono::seconds>(
            "m_period", std::chrono::seconds(1),
            "Period of scheduling", true, __FILE__, __LINE__);
    m_forceQueue = registerProperty<bool>(
            "m_forceQueue", false,
            "Force usage of queue", true, __FILE__, __LINE__);
    m_commitRate = registerProperty<std::size_t>(
            "m_commitRate", static_cast<std::size_t>(100),
            "Commit Rate", true, __FILE__, __LINE__);
}

void HistoryService::onInitialize() {
    m_schedulerService->schedule_after(m_period->getValue(), [&]() { run(); });
}

void HistoryService::onShutdown() {
    m_running.store(false);
}

void HistoryService::run() {
    std::vector<HistoryEntry> temp{};
    while (true) {
        bool isEmpty = false;
        {
            std::lock_guard<std::mutex> const locker(m_historyEntriesMutex);
            isEmpty = m_historyEntries.empty();
            if (!isEmpty) {
                temp.push_back(m_historyEntries.front());
                m_historyEntries.erase(m_historyEntries.begin());
            }
        }
        if (temp.size() < m_commitRate->getValue() && !isEmpty) {
            continue;
        }
        m_historyRepository->insertOf(temp);
        temp.clear();
        break;
    }
    std::optional<Message> message = m_historyMessageQueue->tryReceiveOf();
    while (message.has_value() && m_running) {
        auto historyMessage = message->as<HistoryMessage>();
        HistoryEntry const historyEntry(std::string(&historyMessage.processName[0]), std::string(&historyMessage.serviceName[0]), std::string(&historyMessage.label[0]),
                                        std::string(&historyMessage.text[0]),
                                        std::chrono::time_point_cast<std::chrono::microseconds>(
                                            std::chrono::system_clock::from_time_t(historyMessage.createdTimestamp) +
                                            std::chrono::microseconds(historyMessage.createdTimestampMics)));
        temp.push_back(historyEntry);
        message = m_historyMessageQueue->tryReceiveOf();
        if (!message.has_value() || !m_running || temp.size() >= m_commitRate->getValue()) {
            break;
        }
    }

    if (!temp.empty()) {
        m_historyRepository->insertOf(temp);
    }
    if (m_running) {
        m_schedulerService->schedule_after(m_period->getValue(), [&]() { run(); });
    }
}

void HistoryService::historizeOf(std::vector<HistoryEntry> entries) {
    if (entries.empty()) {
        LOG_ERROR("no entries for hisitorization");
        return;
    }
    if (!m_historyMessageQueue->isOwner() || m_forceQueue->getValue()) {
        for (const auto &entry: entries) {
            HistoryMessage historyMessage{};
            auto len = std::min(entry.getProcessName().size(), sizeof(historyMessage.processName) - 1);
            std::copy(entry.getProcessName().begin(), entry.getProcessName().begin() + static_cast<std::ptrdiff_t>(len),
                      std::begin(historyMessage.processName));
            *(std::begin(historyMessage.processName) + static_cast<std::ptrdiff_t>(len)) = '\0';
            len = std::min(entry.getLabel().size(), sizeof(historyMessage.label) - 1);
            std::copy(entry.getLabel().begin(), entry.getLabel().begin() + static_cast<std::ptrdiff_t>(len),
                      std::begin(historyMessage.label));
            *(std::begin(historyMessage.label) + static_cast<std::ptrdiff_t>(len)) = '\0';
            len = std::min(entry.getServiceName().size(), sizeof(historyMessage.serviceName) - 1);
            std::copy(entry.getServiceName().begin(), entry.getServiceName().begin() + static_cast<std::ptrdiff_t>(len),
                      std::begin(historyMessage.serviceName));
            *(std::begin(historyMessage.serviceName) + static_cast<std::ptrdiff_t>(len)) = '\0';
            historyMessage.createdTimestamp = std::chrono::system_clock::to_time_t(entry.getCreatedTimestamp());
            len = std::min(entry.getText().size(), sizeof(historyMessage.text) - 1);
            std::copy(entry.getText().begin(), entry.getText().begin() + static_cast<std::ptrdiff_t>(len),
                      std::begin(historyMessage.text));
            *(std::begin(historyMessage.text) + static_cast<std::ptrdiff_t>(len)) = '\0';
            historyMessage.createdTimestampMics = std::chrono::duration_cast<std::chrono::microseconds>(
                    entry.getCreatedTimestamp() -
                    std::chrono::system_clock::from_time_t(
                            historyMessage.createdTimestamp)).count();
            Message message = Message(m_historyMessageQueue->getName(), m_processName->getProcessName());
            message.assign(historyMessage);
            m_historyMessageQueue->sendOf(message);
        }
        return;
    }
    std::lock_guard<std::mutex> const locker(m_historyEntriesMutex);
    m_historyEntries.insert(m_historyEntries.end(), entries.begin(),
                            entries.end());
    LOG_TRACE("historize {} {}", entries.size(), entries[0].getServiceName());
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
    try {
        // make sure that all available HistoryEntries are persisted
        run();
    } catch (std::exception &ex) {
        LOG_ERROR("{}", ex.what());
    }
}
