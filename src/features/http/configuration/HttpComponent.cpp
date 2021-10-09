#include "base_library/features/http/configuration/HttpComponent.h"

#include <base_library/core/utils/TypeName.h>
#include <base_library/features/base/configuration/ConfigurationException.h>

#include "base_library/features/http/configuration/HttpEntry.h"

HttpComponent::Shapes HttpComponent::shape{};

HttpComponent::HttpComponent() : Component(shape.CONFIG_ROOT) {}

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
    if (std::strcmp(httpElement->Name(), shape.SERVER_ROOT.c_str()) != 0 &&
        std::strcmp(httpElement->Name(), shape.CLIENT_ROOT.c_str()) != 0) {
      continue;
    }

    bool isClient =
        std::strcmp(httpElement->Name(), shape.CLIENT_ROOT.c_str()) == 0;
    const char *host = httpElement->Attribute(shape.HOST.c_str());
    const char *port = httpElement->Attribute(shape.PORT.c_str());
    const char *certPath = httpElement->Attribute(shape.CERT_PATH.c_str());
    const char *keyPath = httpElement->Attribute(shape.KEY_PATH.c_str());
    const char *readTimeOut =
        httpElement->Attribute(shape.READ_TIMEOUT.c_str());
    const char *writeTimeOut =
        httpElement->Attribute(shape.WRITE_TIMEOUT.c_str());
    const char *connectionTimeOut =
        httpElement->Attribute(shape.CONNECTION_TIMEOUT.c_str());
    const char *idleTimeout =
        httpElement->Attribute(shape.IDLE_TIMEOUT.c_str());

    int32_t lineNumber = httpElement->GetLineNum() + lineOffset - 1;

    if (host == nullptr) {
      throw ConfigurationException(getConfigRoot(), "host is null", lineNumber);
    }

    if (port == nullptr) {
      throw ConfigurationException(getConfigRoot(), "port is null", lineNumber);
    }

    if (isClient && idleTimeout != nullptr) {
      throw ConfigurationException(
          getConfigRoot(), "idle timeout for client defined which is unused",
          lineNumber);
    }

    if (!isClient && connectionTimeOut != nullptr) {
      throw ConfigurationException(
          getConfigRoot(),
          "connection tiemout for server defined which is unused", lineNumber);
    }

    if (readClientConfiguration && isClient) {
      throw ConfigurationException(
          getConfigRoot(), "no double configuration for client", lineNumber);
    }

    if (readServerConfiguration && !isClient) {
      throw ConfigurationException(
          getConfigRoot(), "no double configuration for server", lineNumber);
    }

    if (isClient) {
      readClientConfiguration = true;
    } else {
      readServerConfiguration = true;
    }

    if (isClient) {
      std::shared_ptr<ClientConfiguration> clientConfiguration =
          std::make_shared<ClientConfiguration>(
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
      std::shared_ptr<HttpEntry> httpEntry = std::make_shared<HttpEntry>(
          type_name<HttpComponent>(), clientConfiguration);
      httpEntries.push_back(httpEntry);
    } else {
      std::shared_ptr<ServerConfiguration> serverConfiguration =
          std::make_shared<ServerConfiguration>(
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
      std::shared_ptr<HttpEntry> httpEntry = std::make_shared<HttpEntry>(
          type_name<HttpComponent>(), serverConfiguration);
      httpEntries.push_back(httpEntry);
    }
  }
  return httpEntries;
}