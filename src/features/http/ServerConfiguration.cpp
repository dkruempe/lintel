#include "base_library/features/http/ServerConfiguration.h"
ServerConfiguration::ServerConfiguration(
        std::string host, const int32_t port,
        const std::chrono::milliseconds &readTimeOut,
        const std::chrono::milliseconds &writeTimeOut,
        const std::chrono::milliseconds &idleInterval,
        std::filesystem::path certFile, std::filesystem::path keyFile)
    : m_host(std::move(host)), m_port(port), m_readTimeOut(readTimeOut),
      m_writeTimeOut(writeTimeOut), m_idleInterval(idleInterval),
      m_certFile(std::move(certFile)), m_keyFile(std::move(keyFile)) {}
const std::string &ServerConfiguration::getHost() const { return m_host; }
int32_t ServerConfiguration::getPort() const { return m_port; }
const std::chrono::milliseconds &ServerConfiguration::getReadTimeOut() const {
    return m_readTimeOut;
}
const std::chrono::milliseconds &ServerConfiguration::getWriteTimeOut() const {
    return m_writeTimeOut;
}
const std::chrono::milliseconds &ServerConfiguration::getIdleInterval() const {
    return m_idleInterval;
}
const std::filesystem::path &ServerConfiguration::getCertFile() const {
    return m_certFile;
}
const std::filesystem::path &ServerConfiguration::getKeyFile() const {
    return m_keyFile;
}
std::ostream &operator<<(std::ostream &os,
                         const ServerConfiguration &configuration) {
  os << "m_host: " << configuration.m_host
     << " m_port: " << configuration.m_port
     << " m_readTimeOut: " << configuration.m_readTimeOut.count()
     << " m_writeTimeOut: " << configuration.m_writeTimeOut.count()
     << " m_idleInterval: " << configuration.m_idleInterval.count()
     << " m_certFile: " << configuration.m_certFile
     << " m_keyFile: " << configuration.m_keyFile;
  return os;
}
