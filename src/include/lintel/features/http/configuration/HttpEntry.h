#ifndef LINTEL_HTTPENTRY_H
#define LINTEL_HTTPENTRY_H

#include <memory>
#include <ostream>

#include "ClientConfiguration.h"
#include "ServerConfiguration.h"
#include "lintel/features/base/configuration/Entry.h"

/** Configuration entry holding either a server or client HTTP configuration */
class HttpEntry : public Entry {
private:
    std::shared_ptr<ServerConfiguration> m_serverConfiguration;
    std::shared_ptr<ClientConfiguration> m_clientConfiguration;

public:
    /** @param component component name
     *  @param serverConfiguration server-side configuration */
    HttpEntry(std::string_view component,
              std::shared_ptr<ServerConfiguration> serverConfiguration);

    /** @param component component name
     *  @param clientConfiguration client-side configuration */
    HttpEntry(std::string_view component,
              std::shared_ptr<ClientConfiguration> clientConfiguration);

    /** @return the server configuration, if this is a server entry */
    [[nodiscard]] const std::shared_ptr<ServerConfiguration>
    &getServerConfiguration() const;

    /** @return the client configuration, if this is a client entry */
    [[nodiscard]] const std::shared_ptr<ClientConfiguration>
    &getClientConfiguration() const;

    /** @return true if this is a server entry */
    bool isServer();

    /** @return true if this is a client entry */
    bool isClient();

    friend std::ostream &operator<<(std::ostream &os, const HttpEntry &entry);
};

#endif  // LINTEL_HTTPENTRY_H
