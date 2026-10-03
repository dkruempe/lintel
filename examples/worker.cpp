#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <unistd.h>
#include <utility>

#include <base_library/core/StartupBuilder.h>
#include <base_library/core/services/AbstractService.h>
#include <base_library/core/services/LoggerService.h>
#include <base_library/features/Feature.h>
#include <base_library/features/Features.h>
#include <base_library/features/base/BaseFeature.h>
#include <base_library/features/base/models/HistoryEntry.h>
#include <base_library/features/base/models/ProcessName.h>
#include <base_library/features/base/services/EventBusService.h>
#include <base_library/features/base/services/IHistoryService.h>
#include <base_library/features/property/PropertyFeature.h>
#include <base_library/features/property/repositories/SharedMemoryPropertyRepository.h>
#include <base_library/features/property/services/PropertyService.h>

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

/**
 * Worker process started by the main process through ProcessService.
 *
 * The worker never touches the database: property values are loaded from the
 * shared memory mirror (shm_property) and change notifications are delivered
 * through the event bus (shm_eventbus). ProcessService selects the bootstrap
 * config of this process through the BOOTSTRAP_CONFIG_NAME environment
 * variable (config attribute of the <Process> entry).
 */
class WorkerService : public AbstractService<WorkerService> {
private:
    std::shared_ptr<Hypodermic::Container> m_container;
    std::shared_ptr<ProcessName> m_processName;
    std::atomic_bool m_running = true;
    std::thread m_listener;
    std::thread m_historyThread;

public:
    WorkerService(std::shared_ptr<Hypodermic::Container> container,
                  std::shared_ptr<ProcessName> processName)
            : AbstractService<WorkerService>(processName->getProcessName()),
              m_container(std::move(container)),
              m_processName(std::move(processName)) {}

    void onInitialize() override {
        auto propertyService = m_container->resolve<PropertyService>();
        auto sharedMemoryRepository =
                m_container->resolve<SharedMemoryPropertyRepository>();
        auto eventBusService = m_container->resolve<EventBusService>();

        // unique subscriber name per worker instance (pid based)
        const std::string subscriber =
                m_processName->getProcessName() + "-" + std::to_string(::getpid());
        propertyService->setPropertyChangeBus(eventBusService->of("main"),
                                              subscriber);

        // read the current value from the shared memory mirror before the local
        // property is registered, so that a stale default is never written back
        int32_t initialValue = 2;
        try {
            auto mirrored = sharedMemoryRepository->allOf(
                    "main", "SchedulerService", "__DEFAULT", "numberOfThreads");
            if (!mirrored.empty()) {
                initialValue = std::stoi(mirrored.front()->toString());
            }
        } catch (const std::exception &exception) {
            LOG_WARN("cannot read numberOfThreads from shared memory: {}",
                     exception.what());
        }
        auto numberOfThreads = propertyService->getOrCreate<int32_t>(
                "numberOfThreads", "__DEFAULT", "SchedulerService", "main",
                "number of worker threads", true, initialValue);
        LOG_INFO("{}: numberOfThreads = {} (shared memory mirror)", subscriber,
                 numberOfThreads->getValue());

        // mirror a runtime-changeable property of the main process so that a
        // change made over HTTP is applied by the worker through the event bus
        int32_t loginTimeout = 5;
        try {
            auto mirrored = sharedMemoryRepository->allOf(
                    "main", "AuthService", "__DEFAULT", "m_timeoutLogin");
            if (!mirrored.empty()) {
                loginTimeout = std::stoi(mirrored.front()->toString());
            }
        } catch (const std::exception &exception) {
            LOG_WARN("cannot read m_timeoutLogin from shared memory: {}",
                     exception.what());
        }
        auto timeoutLogin = propertyService->getOrCreate<int32_t>(
                "m_timeoutLogin", "__DEFAULT", "AuthService", "main",
                "timeout of user login in minutes (mirrored)", true, loginTimeout);
        LOG_INFO("{}: m_timeoutLogin = {} (shared memory mirror)", subscriber,
                 timeoutLogin->getValue());

        m_listener = std::thread([this, propertyService, subscriber]() {
            while (m_running) {
                const std::size_t applied =
                        propertyService->applyChangeNotifications();
                if (applied > 0) {
                    try {
                        auto property = propertyService->get(
                                "numberOfThreads", "__DEFAULT", "SchedulerService",
                                "main");
                        LOG_INFO("{}: applied {} property change(s), "
                                 "numberOfThreads = {}",
                                 subscriber, applied, property->toString());
                    } catch (const std::exception &) {
                        LOG_INFO("{}: applied {} property change(s)", subscriber,
                                 applied);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        LOG_INFO("{}: listening for property changes", subscriber);

        // produce a few test history entries every 3 seconds so that main
        // picks them up from the IPC queue and persists them
        auto historyService = m_container->resolve<IHistoryService>();
        const std::string workerPid = std::to_string(::getpid());
        m_historyThread = std::thread(
                [this, historyService, subscriber, workerPid]() {
                    int counter = 0;
                    while (m_running) {
                        ++counter;
                        std::string text =
                                subscriber + " heartbeat #" + std::to_string(counter);
                        HistoryEntry entry(m_processName->getProcessName(),
                                           "WorkerService", "WORKER_HEARTBEAT",
                                           text,
                                           std::chrono::time_point_cast<
                                                   std::chrono::microseconds>(
                                                   std::chrono::system_clock::now()));
                        historyService->historizeOf({entry});
                        LOG_INFO("{}: historized WORKER_HEARTBEAT #{}",
                                 subscriber, counter);
                        for (int i = 0; i < 30 && m_running; ++i) {
                            std::this_thread::sleep_for(
                                    std::chrono::milliseconds(100));
                        }
                    }
                });
    }

    void onShutdown() override {
        m_running = false;
        if (m_listener.joinable()) {
            m_listener.join();
        }
        if (m_historyThread.joinable()) {
            m_historyThread.join();
        }
    }
};

/** Feature that registers the worker service (SHM mirror + event bus only). */
class WorkerFeature : public FeatureInterface {
public:
    explicit WorkerFeature(std::shared_ptr<Features> /*features*/) {}

    void registerTypes(Hypodermic::ContainerBuilder &builder) override {
        builder.registerType<WorkerService>()
                .as<AbstractServiceInterface>()
                .asSelf()
                .singleInstance();
    }

    void initialize(std::shared_ptr<Hypodermic::Container> /*container*/) override {
    }

    std::string_view getName() override { return "Worker"; }
};

int main(int argc, char *argv[]) {
    std::shared_ptr<StartupBuilder> builder = StartupBuilder::with(argc, argv);
    builder->addFeature<BaseFeature>();
    builder->addFeature<PropertyFeature>();
    builder->addFeature<WorkerFeature>();
    builder->start();
    return 0;
}
