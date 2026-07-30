#ifndef CPP_BASE_LIBRARY_HTTP_FEATURE_H
#define CPP_BASE_LIBRARY_HTTP_FEATURE_H

#include "base_library/features/Feature.h"
#include "base_library/features/http/provider/ServerProvider.h"

/** Feature that registers and initializes the HTTP server and client infrastructure */
class HttpFeature : public Feature<Features::Value> {
private:
    std::shared_ptr<ServerProvider> m_ServerProvider;

public:
    /** @param features parent features container */
    HttpFeature(std::shared_ptr<Features> features);

    /** Register HTTP types in the dependency injection container */
    void registerTypes(Hypodermic::ContainerBuilder &builder) override;

    /** Initialize the HTTP server from the container */
    void initialize(std::shared_ptr<Hypodermic::Container> container) override;
};

#endif  // CPP_BASE_LIBRARY_HTTP_FEATURE_H
