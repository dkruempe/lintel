#ifndef CPP_BASE_LIBRARY_BASEFEATURE_H
#define CPP_BASE_LIBRARY_BASEFEATURE_H

#include "base_library/features/Feature.h"

class BaseFeature : public Feature<Features::Value> {
public:
    BaseFeature(std::shared_ptr<Features> features);

    void registerTypes(Hypodermic::ContainerBuilder &builder) override;

    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_BASEFEATURE_H
