#ifndef CPP_BASE_LIBRARY_BASEFEATURE_H
#define CPP_BASE_LIBRARY_BASEFEATURE_H

#include "base_library/features/Feature.h"

/** Base feature implementation that registers core library services */
class BaseFeature : public Feature<Features::Value> {
public:
    /** Construct the base feature
     * @param features The central Features manager */
    BaseFeature(std::shared_ptr<Features> features);

    /** Register core library types in the DI container */
    void registerTypes(Hypodermic::ContainerBuilder &builder) override;

    /** Register core library types in the DI container.
     *
     * Registers the HistoryService only in the process that the configuration
     * explicitly declares as its owner; all other processes receive a
     * NoopHistoryService and therefore never run a history service.
     *
     * @param builder the DI container builder
     * @param configuration the parsed application configuration
     * @param processName the process name of the running process */
    void registerTypes(
            Hypodermic::ContainerBuilder &builder,
            const std::shared_ptr<Configuration> &configuration,
            const std::shared_ptr<ProcessName> &processName) override;

    /** Initialize the base feature with the resolved container */
    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_BASEFEATURE_H
