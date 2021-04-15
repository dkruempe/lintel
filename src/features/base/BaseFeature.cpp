#include "base_library/features/base/BaseFeature.h"
#include "base_library/features/base/services/ExecutorService.h"
#include "base_library/features/base/services/InitializeService.h"
//#include "base_library/features/base/services/ProcessService.h"
#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/base/configuration/ConnectionComponent.h"
#include "base_library/features/base/configuration/PropertyComponent.h"
#include "base_library/features/base/services/SchedulerService.h"

BaseFeature::BaseFeature() : Feature(type_name<BaseFeature>()) {}

void BaseFeature::initialize(std::shared_ptr<Hypodermic::Container> container) {
}

void BaseFeature::registerTypes(Hypodermic::ContainerBuilder &builder) {
  builder.registerType<SchedulerService>()
      .as<AbstractServiceInterface>()
      .asSelf()
      .singleInstance();
  builder.registerType<InitializeService>().singleInstance();
  builder.registerType<ExecutorService>();
  builder.registerType<PropertyComponent>()
      .as<Component>()
      .asSelf()
      .singleInstance();
  builder.registerType<ConnectionComponent>()
      .as<Component>()
      .asSelf()
      .singleInstance();
  builder.registerType<Configuration>().singleInstance();
  //  builder.registerType<ProcessService>();
}