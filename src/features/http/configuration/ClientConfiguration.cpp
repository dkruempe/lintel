#include "base_library/features/http/configuration/ClientConfiguration.h"

#include <utility>

ClientConfiguration::ClientConfiguration(
        std::string host, int32_t port, std::chrono::milliseconds readTimeOut,
        std::chrono::milliseconds writeTimeOut,
        std::chrono::milliseconds connectionTimeout, std::filesystem::path certFile,
        std::filesystem::path keyFile, std::filesystem::path caCertFile)
        : m_host(std::move(host)),
          m_port(port),
          m_readTimeOut(readTimeOut),
          m_writeTimeOut(writeTimeOut),
          m_connectionTimeout(connectionTimeout),
          m_certFile(std::move(certFile)),
          m_keyFile(std::move(keyFile)),
          m_caCertFile(std::move(caCertFile)) {}

const std::string &ClientConfiguration::getHost() const { return m_host; }

int32_t ClientConfiguration::getPort() const { return m_port; }

const std::chrono::milliseconds &ClientConfiguration::getReadTimeOut() const {
    return m_readTimeOut;
}

const std::chrono::milliseconds &ClientConfiguration::getWriteTimeOut() const {
    return m_writeTimeOut;
}

const std::chrono::milliseconds &ClientConfiguration::getConnectionTimeout()
const {
    return m_connectionTimeout;
}

const std::filesystem::path &ClientConfiguration::getCertFile() const {
    return m_certFile;
}

const std::filesystem::path &ClientConfiguration::getKeyFile() const {
    return m_keyFile;
}

const std::filesystem::path &ClientConfiguration::getCaCertFile() const {
    return m_caCertFile;
}

std::ostream &operator<<(std::ostream &os,
                         const ClientConfiguration &configuration) {
    os << "m_host: " << configuration.m_host
       << " m_port: " << configuration.m_port
       << " m_readTimeOut: " << configuration.m_readTimeOut.count()
       << " m_writeTimeOut: " << configuration.m_writeTimeOut.count()
       << " m_connectionTimeout: " << configuration.m_connectionTimeout.count()
       << " m_certFile: " << configuration.m_certFile
       << " m_keyFile: " << configuration.m_keyFile
       << " m_caCertFile: " << configuration.m_caCertFile;
    return os;
}
