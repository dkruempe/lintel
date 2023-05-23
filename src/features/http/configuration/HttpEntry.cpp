#include "base_library/features/http/configuration/HttpEntry.h"

#include <utility>

HttpEntry::HttpEntry(std::string_view component,
                     std::shared_ptr<ServerConfiguration> serverConfiguration)
        : Entry(component),
          m_serverConfiguration(std::move(serverConfiguration)),
          m_clientConfiguration(nullptr) {}

HttpEntry::HttpEntry(std::string_view component,
                     std::shared_ptr<ClientConfiguration> clientConfiguration)
        : Entry(component),
          m_serverConfiguration(nullptr),
          m_clientConfiguration(std::move(clientConfiguration)) {}

std::ostream &operator<<(std::ostream &os, const HttpEntry &entry) {
    os << static_cast<const Entry &>(entry) << " ";
    if (entry.m_clientConfiguration != nullptr) {
        os << entry.m_clientConfiguration;
    } else {
        os << entry.m_serverConfiguration;
    }
    return os;
}

const std::shared_ptr<ServerConfiguration> &HttpEntry::getServerConfiguration()
const {
    return m_serverConfiguration;
}

const std::shared_ptr<ClientConfiguration> &HttpEntry::getClientConfiguration()
const {
    return m_clientConfiguration;
}

bool HttpEntry::isServer() { return m_serverConfiguration != nullptr; }

bool HttpEntry::isClient() { return m_clientConfiguration != nullptr; }
