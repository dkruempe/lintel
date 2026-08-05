#include "base_library/features/http/configuration/HttpComponent.h"

#include <base_library/core/configuration/ConfigurationException.h>

#include <filesystem>

#include <algorithm>

#include "base_library/features/http/configuration/HttpEntry.h"
import base_library.core.utils;
import base_library.core.utils.type_name;

HttpComponent::Shapes HttpComponent::shape{};

HttpComponent::HttpComponent() : Component(shape.CONFIG_ROOT) {}

HttpComponent::HttpComponent(
        std::shared_ptr<EnvironmentConfiguration> environmentConfiguration)
        : Component(shape.CONFIG_ROOT),
          m_environmentConfiguration(std::move(environmentConfiguration)) {}

namespace {

bool parseBool(const char *value) {
    if (value == nullptr) {
        return false;
    }
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    return normalized == "true" || normalized == "1" || normalized == "yes";
}

std::string trimmed(const std::string &value) {
    const std::string whitespace = " \t\r\n";
    const std::size_t first = value.find_first_not_of(whitespace);
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(whitespace);
    return value.substr(first, last - first + 1);
}

std::vector<std::string> parseTrustedProxies(const char *value) {
    std::vector<std::string> proxies;
    if (value == nullptr) {
        return proxies;
    }
    for (const std::string &entry: StringUtils::split(value, ',')) {
        const std::string entryTrimmed = trimmed(entry);
        if (!entryTrimmed.empty()) {
            proxies.push_back(entryTrimmed);
        }
    }
    return proxies;
}

std::string resolveRelativePath(
        const std::string &path,
        const std::shared_ptr<EnvironmentConfiguration> &environmentConfiguration) {
    if (path.empty() || environmentConfiguration == nullptr) {
        return path;
    }
    std::filesystem::path filePath(path);
    if (filePath.is_absolute()) {
        return filePath.string();
    }
    std::filesystem::path configDirectory(
            environmentConfiguration->of(EnvironmentConfiguration::ConfigDirectory));
    return (configDirectory / filePath).string();
}

std::shared_ptr<Entry> createClientEntry(
        const char *host, const char *port,
        const char *readTimeOut, const char *writeTimeOut,
        const char *connectionTimeOut, const std::string &certPath,
        const std::string &keyPath, const std::string &caCertPath) {
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
            certPath,
            keyPath,
            caCertPath);
    return std::make_shared<HttpEntry>(
            type_name<HttpComponent>(), clientConfiguration);
}

std::shared_ptr<Entry> createServerEntry(
        const char *host, const char *port,
        const char *readTimeOut, const char *writeTimeOut,
        const char *idleTimeout, const std::string &certPath,
        const std::string &keyPath, bool requireTls,
        std::vector<std::string> trustedProxies) {
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
            certPath,
            keyPath,
            requireTls,
            std::move(trustedProxies));
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
        const char *caCertPath = httpElement->Attribute(shape.CA_CERT_PATH);
        const char *readTimeOut =
                httpElement->Attribute(shape.READ_TIMEOUT);
        const char *writeTimeOut =
                httpElement->Attribute(shape.WRITE_TIMEOUT);
        const char *connectionTimeOut =
                httpElement->Attribute(shape.CONNECTION_TIMEOUT);
        const char *idleTimeout =
                httpElement->Attribute(shape.IDLE_TIMEOUT);

        bool requireTls = parseBool(httpElement->Attribute(shape.REQUIRE_TLS));

        int32_t lineNumber = httpElement->GetLineNum() + lineOffset - 1;

        validateHttpElement(isClient, host, port, idleTimeout,
                            connectionTimeOut, readClientConfiguration,
                            readServerConfiguration, getConfigRoot(),
                            lineNumber);

        if (isClient) {
            readClientConfiguration = true;
            httpEntries.push_back(createClientEntry(
                    host, port, readTimeOut, writeTimeOut,
                    connectionTimeOut,
                    resolveRelativePath(certPath != nullptr ? certPath : "",
                                        m_environmentConfiguration),
                    resolveRelativePath(keyPath != nullptr ? keyPath : "",
                                        m_environmentConfiguration),
                    resolveRelativePath(caCertPath != nullptr ? caCertPath : "",
                                        m_environmentConfiguration)));
        } else {
            readServerConfiguration = true;
            httpEntries.push_back(createServerEntry(
                    host, port, readTimeOut, writeTimeOut,
                    idleTimeout,
                    resolveRelativePath(certPath != nullptr ? certPath : "",
                                        m_environmentConfiguration),
                    resolveRelativePath(keyPath != nullptr ? keyPath : "",
                                        m_environmentConfiguration),
                    requireTls,
                    parseTrustedProxies(
                            httpElement->Attribute(shape.TRUSTED_PROXIES))));
        }
    }
    return httpEntries;
}