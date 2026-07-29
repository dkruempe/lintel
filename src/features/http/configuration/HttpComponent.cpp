#include "base_library/features/http/configuration/HttpComponent.h"

#include <base_library/core/utils/TypeName.h>
#include <base_library/features/base/configuration/ConfigurationException.h>

#include "base_library/features/http/configuration/HttpEntry.h"

HttpComponent::Shapes HttpComponent::shape{};

HttpComponent::HttpComponent() : Component(shape.CONFIG_ROOT) {}

namespace {

std::shared_ptr<Entry> createClientEntry(
        const char *host, const char *port,
        const char *readTimeOut, const char *writeTimeOut,
        const char *connectionTimeOut, const char *certPath,
        const char *keyPath) {
    auto clientConfiguration = std::make_shared<ClientConfiguration>(
            host, std::stoi(port),
            readTimeOut != nullptr
            ? std::chrono::milliseconds(std::stoi(readTimeOut))
            : std::chrono::milliseconds(0),
            writeTimeOut != nullptr
            ? std::chrono::milliseconds(std::stoi(writeTimeOut))
            : std::chrono::milliseconds(0),
            connectionTimeOut != nullptr
            ? std::chrono::milliseconds(std::stoi(connectionTimeOut))
            : std::chrono::milliseconds(0),
            certPath != nullptr ? certPath : "",
            keyPath != nullptr ? keyPath : "");
    return std::make_shared<HttpEntry>(
            type_name<HttpComponent>(), clientConfiguration);
}

std::shared_ptr<Entry> createServerEntry(
        const char *host, const char *port,
        const char *readTimeOut, const char *writeTimeOut,
        const char *idleTimeout, const char *certPath,
        const char *keyPath) {
    auto serverConfiguration = std::make_shared<ServerConfiguration>(
            host, std::stoi(port),
            readTimeOut != nullptr
            ? std::chrono::milliseconds(std::stoi(readTimeOut))
            : std::chrono::milliseconds(0),
            writeTimeOut != nullptr
            ? std::chrono::milliseconds(std::stoi(writeTimeOut))
            : std::chrono::milliseconds(0),
            idleTimeout != nullptr
            ? std::chrono::milliseconds(std::stoi(idleTimeout))
            : std::chrono::milliseconds(0),
            certPath != nullptr ? certPath : "",
            keyPath != nullptr ? keyPath : "");
    return std::make_shared<HttpEntry>(
            type_name<HttpComponent>(), serverConfiguration);
}

void validateHttpElement(bool isClient, const char *host, const char *port,
                         const char *idleTimeout,
                         const char *connectionTimeOut,
                         bool readClientConfiguration,
                         bool readServerConfiguration,
                         const std::string &configRoot, int32_t lineNumber) {
    if (host == nullptr) {
        throw ConfigurationException(configRoot, "host is null", lineNumber);
    }
    if (port == nullptr) {
        throw ConfigurationException(configRoot, "port is null", lineNumber);
    }
    if (isClient && idleTimeout != nullptr) {
        throw ConfigurationException(
                configRoot, "idle timeout for client defined which is unused",
                lineNumber);
    }
    if (!isClient && connectionTimeOut != nullptr) {
        throw ConfigurationException(
                configRoot,
                "connection timeout for server defined which is unused", lineNumber);
    }
    if (readClientConfiguration && isClient) {
        throw ConfigurationException(
                configRoot, "no double configuration for client", lineNumber);
    }
    if (readServerConfiguration && !isClient) {
        throw ConfigurationException(
                configRoot, "no double configuration for server", lineNumber);
    }
}

} // namespace

std::vector<std::shared_ptr<Entry>> HttpComponent::parse(
        const std::string &content, const std::string &fileName,
        const int32_t lineOffset) {
    std::vector<std::shared_ptr<Entry>> httpEntries;
    tinyxml2::XMLDocument document;
    document.Parse(content.c_str());

    tinyxml2::XMLElement *rootNode =
            document.FirstChildElement(getConfigRoot().c_str());
    if (rootNode == nullptr) {
        return httpEntries;
    }

    bool readClientConfiguration = false;
    bool readServerConfiguration = false;
    for (tinyxml2::XMLElement *httpElement = rootNode->FirstChildElement();
         httpElement != nullptr;
         httpElement = httpElement->NextSiblingElement()) {
        if (std::strcmp(httpElement->Name(), shape.SERVER_ROOT) != 0 &&
            std::strcmp(httpElement->Name(), shape.CLIENT_ROOT) != 0) {
            continue;
        }

        bool isClient =
                std::strcmp(httpElement->Name(), shape.CLIENT_ROOT) == 0;
        const char *host = httpElement->Attribute(shape.HOST);
        const char *port = httpElement->Attribute(shape.PORT);
        const char *certPath = httpElement->Attribute(shape.CERT_PATH);
        const char *keyPath = httpElement->Attribute(shape.KEY_PATH);
        const char *readTimeOut =
                httpElement->Attribute(shape.READ_TIMEOUT);
        const char *writeTimeOut =
                httpElement->Attribute(shape.WRITE_TIMEOUT);
        const char *connectionTimeOut =
                httpElement->Attribute(shape.CONNECTION_TIMEOUT);
        const char *idleTimeout =
                httpElement->Attribute(shape.IDLE_TIMEOUT);

        int32_t lineNumber = httpElement->GetLineNum() + lineOffset - 1;

        validateHttpElement(isClient, host, port, idleTimeout,
                            connectionTimeOut, readClientConfiguration,
                            readServerConfiguration, getConfigRoot(),
                            lineNumber);

        if (isClient) {
            readClientConfiguration = true;
            httpEntries.push_back(createClientEntry(
                    host, port, readTimeOut, writeTimeOut,
                    connectionTimeOut, certPath, keyPath));
        } else {
            readServerConfiguration = true;
            httpEntries.push_back(createServerEntry(
                    host, port, readTimeOut, writeTimeOut,
                    idleTimeout, certPath, keyPath));
        }
    }
    return httpEntries;
}