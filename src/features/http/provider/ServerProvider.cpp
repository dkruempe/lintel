#include "lintel/features/http/provider/ServerProvider.h"

#include "lintel/features/http/configuration/HttpComponent.h"
#include "lintel/features/http/configuration/HttpEntry.h"

ServerProvider::ServerProvider(
        const std::shared_ptr<Configuration> &configuration,
        const std::vector<std::shared_ptr<Controller>> &controllers)
        : m_server(build(configuration, controllers)) {}

const std::shared_ptr<Server> &ServerProvider::provide() { return m_server; }

std::shared_ptr<Server> ServerProvider::build(
        const std::shared_ptr<Configuration> &configuration,
        const std::vector<std::shared_ptr<Controller>> &controllers) {
    std::vector<std::shared_ptr<Entry>> entries =
            configuration->configurationOf<HttpComponent>();
    std::shared_ptr<Server> server;
    for (const auto &entry: entries) {
        std::shared_ptr<HttpEntry> httpEntry =
                std::static_pointer_cast<HttpEntry>(entry);
        if (httpEntry->isClient()) {
            continue;
        }
        server = std::make_shared<Server>(*httpEntry->getServerConfiguration(),
                                          controllers);
    }
    return server;
}
