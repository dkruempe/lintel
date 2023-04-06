#include "base_library/features/base/BaseFeature.h"

#include <base_library/features/base/command_line/SharedMemoryCliComponent.h>

#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/plugins/DatabaseBootstrapPlugin.h"
#include "base_library/core/plugins/SharedMemoryBootstrapPlugin.h"
#include "base_library/core/plugins/VirtualGroupBootstrapPlugin.h"
#include "base_library/core/services/BootstrapService.h"
#include "base_library/core/services/PersistableService.h"
#include "base_library/core/services/ProcessService.h"
#include "base_library/core/services/SharedMemoryService.h"
#include "base_library/features/base/command_line/CryptionCliComponent.h"
#include "base_library/features/base/command_line/ProcessCliComponent.h"
#include "base_library/features/base/command_line/UserManagementCliComponent.h"
#include "base_library/features/base/controller/ProcessApi.h"
#include "base_library/features/base/controller/ProcessController.h"
#include "base_library/features/base/controller/SharedMemoryApi.h"
#include "base_library/features/base/controller/SharedMemoryController.h"
#include "base_library/features/base/controller/UserApi.h"
#include "base_library/features/base/controller/UserController.h"
#include "base_library/features/base/provider/GroupProvider.h"
#include "base_library/features/base/repositories/GroupRepository.h"
#include "base_library/features/base/repositories/HistoryRepository.h"
#include "base_library/features/base/repositories/UserRepository.h"
#include "base_library/features/base/services/HistoryService.h"
#include "base_library/features/base/services/AuthService.h"
#include "base_library/features/base/services/ExecutorService.h"
#include "base_library/features/base/services/InitializeService.h"
#include "base_library/features/base/services/ProcessArgumentService.h"
#include "base_library/features/base/services/SchedulerService.h"
#include "base_library/features/base/services/SharedMemorySegmentManager.h"

BaseFeature::BaseFeature() : Feature(type_name<BaseFeature>()) {}

void BaseFeature::initialize(std::shared_ptr<Hypodermic::Container> container) {
}

void BaseFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
  builder.registerType<AuthService>()
      .as<AbstractServiceInterface>()
      .asSelf()
      .singleInstance();
  builder.registerType<SchedulerService>()
      .as<AbstractServiceInterface>()
      .asSelf()
      .singleInstance();
  builder.registerType<InitializeService>().singleInstance();
  builder.registerType<ExecutorService>();
  builder.registerType<DatabaseConnectionConfigurations>().singleInstance();
  builder.registerType<CryptionCliComponent>()
      .as<CommandLineComponent>()
      .asSelf()
      .singleInstance();
  builder.registerType<UserManagementCliComponent>()
      .as<CommandLineComponent>()
      .asSelf()
      .singleInstance();
  builder.registerType<BootstrapService>().singleInstance();
  builder.registerType<DatabaseBootstrapPlugin>()
      .as<BootstrapPlugin>()
      .asSelf()
      .singleInstance();
  builder.registerType<VirtualGroupBootstrapPlugin>()
      .as<BootstrapPlugin>()
      .asSelf()
      .singleInstance();
  builder.registerType<SharedMemoryBootstrapPlugin>()
      .as<BootstrapPlugin>()
      .asSelf()
      .singleInstance();
  builder.registerType<PersistableService>().singleInstance();
  builder.registerType<UserApi>().singleInstance();
  builder.registerType<UserController>()
      .as<Controller>()
      .as<GroupProvider>()
      .asSelf()
      .singleInstance();
  builder.registerType<GroupRepository>()
      .as<PersistableBean>()
      .asSelf()
      .singleInstance();
  builder.registerType<UserRepository>().singleInstance();
  builder.registerType<ProcessArgumentService>().singleInstance();
  builder.registerType<SharedMemorySegmentManager>().singleInstance();
  builder.registerType<SharedMemoryService>()
      .as<AbstractServiceInterface>()
      .asSelf()
      .singleInstance();
  builder.registerType<ProcessService>()
      .as<AbstractServiceInterface>()
      .asSelf()
      .singleInstance();
  builder.registerType<ProcessCliComponent>()
      .as<CommandLineComponent>()
      .asSelf()
      .singleInstance();
  builder.registerType<ProcessApi>().singleInstance();
  builder.registerType<ProcessController>()
      .as<Controller>()
      .as<GroupProvider>()
      .asSelf()
      .singleInstance();
  builder.registerType<SharedMemoryController>()
      .as<Controller>()
      .as<GroupProvider>()
      .asSelf()
      .singleInstance();
  builder.registerType<SharedMemoryApi>().singleInstance();
  builder.registerType<SharedMemoryCliComponent>()
      .as<CommandLineComponent>()
      .asSelf()
      .singleInstance();
  builder.registerType<HistoryRepository>().singleInstance();
  builder.registerType<HistoryService>()
      .as<AbstractServiceInterface>()
      .asSelf()
      .singleInstance();
}