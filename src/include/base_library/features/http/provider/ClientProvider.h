#ifndef CPP_BASE_LIBRARY_CLIENTPROVIDER_H
#define CPP_BASE_LIBRARY_CLIENTPROVIDER_H

#include "base_library/core/configuration/Configuration.h"
#include "base_library/features/http/service/Client.h"

/** Provides a shared HTTP client instance built from configuration */
class ClientProvider {
private:
    std::shared_ptr<Client> m_client;

    /** Build a Client from the given configuration */
    static std::shared_ptr<Client> build(
            const std::shared_ptr<Configuration> &configuration);

public:
    explicit ClientProvider(const std::shared_ptr<Configuration> &configuration);

    /** @return the shared Client instance */
    const std::shared_ptr<Client> &provide();
};

#endif  // CPP_BASE_LIBRARY_CLIENTPROVIDER_H
