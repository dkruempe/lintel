#include "lintel/features/base/BaseFeature.h"

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include "lintel/core/persistence/DatabaseConnectionConfigurations.h"
#include "lintel/core/plugins/AdminUserBootstrapPlugin.h"
#include "lintel/core/plugins/DatabaseBootstrapPlugin.h"
#include "lintel/core/plugins/MessageQueueBootstrapPlugin.h"
#include "lintel/core/plugins/SharedMemoryBootstrapPlugin.h"
#include "lintel/core/plugins/SingleInstanceBootstrapPlugin.h"
#include "lintel/core/plugins/VirtualGroupBootstrapPlugin.h"
#include "lintel/core/services/BootstrapService.h"
#include "lintel/core/services/LoggerService.h"
#include "lintel/core/services/PersistableService.h"
#include "lintel/core/services/ISharedMemoryService.h"
#include "lintel/core/services/ProcessService.h"
#include "lintel/core/services/SharedMemoryService.h"
#include "lintel/features/base/configuration/Configuration.h"
#include "lintel/features/base/configuration/HistoryComponent.h"
#include "lintel/features/base/configuration/HistoryServiceEntry.h"
#include "lintel/features/base/models/ProcessName.h"
#include "lintel/features/base/provider/GroupProvider.h"
#include "lintel/features/base/repositories/GroupRepository.h"
#include "lintel/features/base/repositories/HistoryRepository.h"
#include "lintel/features/base/repositories/MessageQueueRepository.h"
#include "lintel/features/base/repositories/IMessageQueueRepository.h"
#include "lintel/features/base/repositories/UserRepository.h"
#include "lintel/features/base/services/AuthService.h"
#include "lintel/features/base/services/EventBusService.h"
#include "lintel/features/base/services/IAuthService.h"
#include "lintel/features/base/services/IHistoryService.h"
#include "lintel/features/base/services/ExecutorService.h"
#include "lintel/features/base/services/HistoryService.h"
#include "lintel/features/base/services/InitializeService.h"
#include "lintel/features/base/services/MessageQueueService.h"
#include "lintel/features/base/services/NoopHistoryService.h"
#include "lintel/features/base/services/ProcessArgumentService.h"
#include "lintel/features/base/services/SchedulerService.h"
#include "lintel/features/base/services/ISharedMemorySegmentManager.h"
#include "lintel/features/base/services/SharedMemorySegmentManager.h"

BaseFeature::BaseFeature(std::shared_ptr<Features> features)
  : Feature(Features::Base, std::move(features)) {}

void BaseFeature::initialize(std::shared_ptr<Hypodermic::Container> container) {}

void BaseFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
    registerTypes(builder, nullptr, nullptr);
}

/**
 * Registers the core services in the DI container.
 *
 * The flow in essence:
 *   1. determine the process name (empty if no ProcessName was passed).
 *   2. register HistoryService as a real service only where the configuration
 *      names this process explicitly as History owner. Every other process gets
 *      a NoopHistoryService instance and therefore never keeps a history.
 *   3. register AuthService, SchedulerService, InitializeService and
 *      ExecutorService as singletons resp. transients.
 *
 * The caller is StartupBuilder during the bootstrap; the null-argument
 * overload calls this variant with empty values and therefore always ends up
 * in the noop branch.
 *
 * @param builder          the DI container builder of the process
 * @param configuration    the parsed application configuration, may be null
 * @param processName      the process name of the running process, may be null
 */
void BaseFeature::registerTypes(
        Hypodermic::ContainerBuilder &builder,
        const std::shared_ptr<Configuration> &configuration,
        const std::shared_ptr<ProcessName> &processName) {
  // empty process name if the component is registered without a ProcessName -
  // this happens when testing the component without bootstrap.
    const std::string processNameString =
            processName != nullptr ? processName->getProcessName() : "";
    bool hasHistoryConfig = false;
    bool isHistoryOwner = false;
  // only the processes the configuration names as History owner get a real
  // history. All others: noop.
    if (configuration != nullptr) {
        auto historyEntries = configuration->configurationOf<HistoryComponent>();
        hasHistoryConfig = !historyEntries.empty();
        for (const auto &entry: historyEntries) {
            const auto historyEntry =
                    std::static_pointer_cast<HistoryServiceEntry>(entry);
            if (historyEntry->get_process_name() == processNameString) {
                isHistoryOwner = true;
                break;
            }
        }
    }
  // two cases to distinguish: configuration present, but this process is not
  // the owner. Then the service is still registered (the other processes write
  // to the history), but read only - hence noop for the rest.
    if (hasHistoryConfig) {
        if (isHistoryOwner) {
            LOG_INFO("register HistoryService for process {} (owner)",
                     processNameString);
        } else {
            LOG_INFO("register HistoryService for process {} (non-owner, "
                     "producer only)",
                     processNameString);
        }
        builder.registerType<HistoryService>()
                .as<AbstractServiceInterface>()
                .as<IHistoryService>()
                .asSelf()
                .singleInstance();
    } else {
    // no history section in the configuration: noop for all processes.
        LOG_INFO("process {} has no history configuration -> "
                 "register NoopHistoryService",
                 processNameString.empty() ? "<unknown>" : processNameString);
        builder.registerType<NoopHistoryService>()
                .as<IHistoryService>()
                .asSelf()
                .singleInstance();
    }
  // auth is identical in every process and is therefore registered without a filter.
    builder.registerType<AuthService>()
    .as<AbstractServiceInterface>()
    .as<IAuthService>()
    .asSelf()
    .singleInstance();
  // scheduler and executor are transient services, InitializeService the entry point.
  builder.registerType<SchedulerService>()
    .as<AbstractServiceInterface>()
    .asSelf()
    .singleInstance();
  builder.registerType<InitializeService>().singleInstance();
  builder.registerType<ExecutorService>();
  builder.registerType<DatabaseConnectionConfigurations>().singleInstance();
  builder.registerType<BootstrapService>().singleInstance();
  builder.registerType<DatabaseBootstrapPlugin>()
    .as<BootstrapPlugin>()
    .asSelf()
    .singleInstance();
  builder.registerType<MessageQueueBootstrapPlugin>()
    .as<BootstrapPlugin>()
    .asSelf()
    .singleInstance();
  builder.registerType<VirtualGroupBootstrapPlugin>()
    .as<BootstrapPlugin>()
    .asSelf()
    .singleInstance();
  builder.registerType<AdminUserBootstrapPlugin>()
    .as<BootstrapPlugin>()
    .asSelf()
    .singleInstance();
  builder.registerType<SharedMemoryBootstrapPlugin>()
    .as<BootstrapPlugin>()
    .asSelf()
    .singleInstance();
  builder.registerType<SingleInstanceBootstrapPlugin>()
    .as<BootstrapPlugin>()
    .asSelf()
    .singleInstance();
  builder.registerType<PersistableService>().singleInstance();
  builder.registerType<GroupRepository>()
    .as<PersistableBean>()
    .asSelf()
    .singleInstance();
  builder.registerType<MessageQueueRepository>()
    .as<IMessageQueueRepository>()
    .asSelf()
    .singleInstance();
  builder.registerType<UserRepository>().singleInstance();
  builder.registerType<ProcessArgumentService>().singleInstance();
  builder.registerType<SharedMemorySegmentManager>()
    .as<ISharedMemorySegmentManager>()
    .asSelf()
    .singleInstance();
  builder.registerType<SharedMemoryService>()
    .as<AbstractServiceInterface>()
    .as<ISharedMemoryService>()
    .asSelf()
    .singleInstance();
  builder.registerType<ProcessService>()
    .as<AbstractServiceInterface>()
    .asSelf()
    .singleInstance();
  builder.registerType<HistoryRepository>().singleInstance();
  builder.registerType<MessageQueueService>()
  .as<AbstractServiceInterface>()
  .asSelf()
  .singleInstance();
  builder.registerType<EventBusService>()
    .as<AbstractServiceInterface>()
    .asSelf()
    .singleInstance();
}