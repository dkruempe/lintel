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

    /** Initialize the base feature with the resolved container */
    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_BASEFEATURE_H
