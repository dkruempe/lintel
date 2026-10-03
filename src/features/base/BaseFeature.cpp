#include "base_library/features/base/BaseFeature.h"

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/AdminUserBootstrapPlugin.h"
#include "base_library/core/plugins/DatabaseBootstrapPlugin.h"
#include "base_library/core/plugins/MessageQueueBootstrapPlugin.h"
#include "base_library/core/plugins/SharedMemoryBootstrapPlugin.h"
#include "base_library/core/plugins/SingleInstanceBootstrapPlugin.h"
#include "base_library/core/plugins/VirtualGroupBootstrapPlugin.h"
#include "base_library/core/services/BootstrapService.h"
#include "base_library/core/services/LoggerService.h"
#include "base_library/core/services/PersistableService.h"
#include "base_library/core/services/ISharedMemoryService.h"
#include "base_library/core/services/ProcessService.h"
#include "base_library/core/services/SharedMemoryService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/HistoryComponent.h"
#include "base_library/features/base/configuration/HistoryServiceEntry.h"
#include "base_library/features/base/models/ProcessName.h"
#include "base_library/features/base/provider/GroupProvider.h"
#include "base_library/features/base/repositories/GroupRepository.h"
#include "base_library/features/base/repositories/HistoryRepository.h"
#include "base_library/features/base/repositories/MessageQueueRepository.h"
#include "base_library/features/base/repositories/IMessageQueueRepository.h"
#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/base/services/EventBusService.h"
#include "base_library/features/base/services/IAuthService.h"
#include "base_library/features/base/services/IHistoryService.h"
#include "base_library/features/base/services/ExecutorService.h"
#include "base_library/features/base/services/HistoryService.h"
#include "base_library/features/base/services/InitializeService.h"
#include "base_library/features/base/services/MessageQueueService.h"
#include "base_library/features/base/services/NoopHistoryService.h"
#include "base_library/features/base/services/ProcessArgumentService.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/ISharedMemorySegmentManager.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"

BaseFeature::BaseFeature(std::shared_ptr<Features> features)
  : Feature(Features::Base, std::move(features)) {}

void BaseFeature::initialize(std::shared_ptr<Hypodermic::Container> container) {}

void BaseFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
    registerTypes(builder, nullptr, nullptr);
}

void BaseFeature::registerTypes(
        Hypodermic::ContainerBuilder &builder,
        const std::shared_ptr<Configuration> &configuration,
        const std::shared_ptr<ProcessName> &processName) {
    const std::string processNameString =
            processName != nullptr ? processName->getProcessName() : "";
    bool hasHistoryConfig = false;
    bool isHistoryOwner = false;
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
        LOG_INFO("process {} has no history configuration -> "
                 "register NoopHistoryService",
                 processNameString.empty() ? "<unknown>" : processNameString);
        builder.registerType<NoopHistoryService>()
                .as<IHistoryService>()
                .asSelf()
                .singleInstance();
    }
    builder.registerType<AuthService>()
    .as<AbstractServiceInterface>()
    .as<IAuthService>()
    .asSelf()
    .singleInstance();
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