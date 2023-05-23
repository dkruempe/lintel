#ifndef CPP_BASE_LIBRARY_COMMAND_LINE_INTERFACE_FEATURE_H
#define CPP_BASE_LIBRARY_COMMAND_LINE_INTERFACE_FEATURE_H

#include "base_library/features/Feature.h"
#include "base_library/features/cli/services/CommandLineService.h"

class CommandLineFeature : public Feature {
private:
    std::shared_ptr<CommandLineService> m_commandLineService;

public:
    CommandLineFeature();

    void registerTypes(Hypodermic::ContainerBuilder &builder) override;

    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_COMMAND_LINE_INTERFACE_FEATURE_H
