#include "base_library/features/cli/CommandLineFeature.h"

#include "Hypodermic/Container.h"
#include "Hypodermic/ContainerBuilder.h"

#include "base_library/features/cli/components/HistoryCliComponent.h"
#include "base_library/features/cli/components/MessageQueueCliComponent.h"
#include "base_library/features/cli/components/ProcessCliComponent.h"
#include "base_library/features/cli/components/SharedMemoryCliComponent.h"
#include "base_library/features/cli/components/UserManagementCliComponent.h"
#include "base_library/features/cli/providers/AuthArgumentProvider.h"
#include "base_library/features/cli/services/AuthCliService.h"
#include "base_library/features/cli/services/InputService.h"
#include "base_library/features/cli/services/TerminalService.h"
#include "base_library/features/cli/utils/CommandLineUtils.h"

CommandLineFeature::CommandLineFeature(std::shared_ptr<Features> features) : Feature(Features::Cli, std::move(features))
{}

void CommandLineFeature::registerTypes(Hypodermic::ContainerBuilder &builder)
{
  builder.registerType<CommandLineService>().as<AbstractServiceInterface>().asSelf().singleInstance();
  builder.registerType<CommandLineHistoryService>().as<AbstractServiceInterface>().asSelf().singleInstance();
  builder.registerType<AuthCliService>().singleInstance();
  builder.registerType<CommandLineUtils>().singleInstance();
  builder.registerType<TerminalService>().as<ITerminalService>().asSelf().singleInstance();
  builder.registerType<InputService>().as<IInputService>().asSelf().singleInstance();
  builder.registerType<AuthArgumentProvider>().as<ArgumentProvider>().asSelf().singleInstance();
  builder.registerType<UserManagementCliComponent>()
    .as<CommandLineComponent>()
    .asSelf()
    .singleInstance();
  builder.registerType<ProcessCliComponent>()
    .as<CommandLineComponent>()
    .asSelf()
    .singleInstance();
  builder.registerType<SharedMemoryCliComponent>()
    .as<CommandLineComponent>()
    .asSelf()
    .singleInstance();
  builder.registerType<MessageQueueCliComponent>()
    .as<CommandLineComponent>()
    .asSelf()
    .singleInstance();
  builder.registerType<HistoryCliComponent>()
    .as<CommandLineComponent>()
    .asSelf()
    .singleInstance();
}

void CommandLineFeature::initialize(std::shared_ptr<Hypodermic::Container> container)
{
  m_commandLineService = container->resolve<CommandLineService>();
}
