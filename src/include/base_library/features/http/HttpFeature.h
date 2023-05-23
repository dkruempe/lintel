#ifndef CPP_BASE_LIBRARY_HTTP_FEATURE_H
#define CPP_BASE_LIBRARY_HTTP_FEATURE_H

#include "base_library/features/Feature.h"
#include "base_library/features/http/provider/ServerProvider.h"

class HttpFeature : public Feature {
private:
    std::shared_ptr<ServerProvider> m_ServerProvider;

public:
    HttpFeature();

    void registerTypes(Hypodermic::ContainerBuilder &builder) override;

    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_HTTP_FEATURE_H
