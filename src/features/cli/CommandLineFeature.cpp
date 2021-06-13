#include "base_library/features/cli/CommandLineFeature.h"
CommandLineFeature::CommandLineFeature()
    : Feature(type_name<CommandLineFeature>()) {}

void CommandLineFeature::registerTypes(
    Hypodermic::ContainerBuilder& builder) {
  builder.registerType<CommandLineService>().singleInstance();
}

void CommandLineFeature::initialize(
    std::shared_ptr<Hypodermic::Container> container) {
  m_commandLineService = container->resolve<CommandLineService>();
}
