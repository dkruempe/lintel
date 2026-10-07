#include "lintel/features/http/provider/ClientProvider.h"

#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"

ClientProvider::ClientProvider(
        const std::shared_ptr<Configuration> &configuration)
        : m_client(build(configuration)) {}

std::shared_ptr<Client> ClientProvider::build(
        const std::shared_ptr<Configuration> &configuration) {
    std::vector<std::shared_ptr<Entry>> entries =
            configuration->configurationOf<HttpComponent>();
    std::shared_ptr<Client> client;
    for (const auto &entry: entries) {
        std::shared_ptr<HttpEntry> httpEntry =
                std::static_pointer_cast<HttpEntry>(entry);
        if (httpEntry->isServer()) {
            continue;
        }
        client = std::make_shared<Client>(httpEntry->getClientConfiguration());
    }
    return client;
}

const std::shared_ptr<Client> &ClientProvider::provide() { return m_client; }
