#include "base_library/features/cli/CommandLineFeature.h"

#include "base_library/features/cli/providers/AuthArgumentProvider.h"
#include "base_library/features/cli/services/AuthCliService.h"
#include "base_library/features/cli/services/InputService.h"
#include "base_library/features/cli/services/TerminalService.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

CommandLineFeature::CommandLineFeature()
    : Feature(type_name<CommandLineFeature>()) {}

void CommandLineFeature::registerTypes(Hypodermic::ContainerBuilder& builder) {
  builder.registerType<CommandLineService>()
      .as<AbstractServiceInterface>()
      .asSelf()
      .singleInstance();
  builder.registerType<AuthCliService>().singleInstance();
  builder.registerType<CommandLineUtils>().singleInstance();
  builder.registerType<TerminalService>().singleInstance();
  builder.registerType<InputService>().singleInstance();
  builder.registerType<AuthArgumentProvider>()
      .as<ArgumentProvider>()
      .asSelf()
      .singleInstance();
}

void CommandLineFeature::initialize(
    std::shared_ptr<Hypodermic::Container> container) {
  m_commandLineService = container->resolve<CommandLineService>();
}
