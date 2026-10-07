#ifndef LINTEL_COMMAND_LINE_INTERFACE_FEATURE_H
#define LINTEL_COMMAND_LINE_INTERFACE_FEATURE_H

#include "lintel/features/Feature.h"
#include "lintel/features/cli/services/CommandLineService.h"

/** Feature that registers and initializes the command-line interface */
class CommandLineFeature : public Feature<Features::Value> {
private:
    std::shared_ptr<CommandLineService> m_commandLineService;

public:
    /** @param features parent features container */
    CommandLineFeature(std::shared_ptr<Features> features);

    /** Register CLI types in the dependency injection container */
    void registerTypes(Hypodermic::ContainerBuilder &builder) override;

    /** Initialize the command-line service from the container */
    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // LINTEL_COMMAND_LINE_INTERFACE_FEATURE_H
