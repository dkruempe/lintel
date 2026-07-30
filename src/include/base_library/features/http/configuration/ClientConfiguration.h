#ifndef HTTP_LIBRARY_CLIENTCONFIGURATION_H
#define HTTP_LIBRARY_CLIENTCONFIGURATION_H

#include <chrono>
#include <filesystem>
#include <ostream>
#include <string>

/** Configuration for an HTTP client (host, port, timeouts, TLS) */
class ClientConfiguration {
private:
    const std::string m_host;
    const int32_t m_port;
    std::chrono::milliseconds m_readTimeOut;
    std::chrono::milliseconds m_writeTimeOut;
    std::chrono::milliseconds m_connectionTimeout;
    std::filesystem::path m_certFile;
    std::filesystem::path m_keyFile;

public:
    /** @param host remote host
     *  @param port remote port
     *  @param readTimeOut read timeout
     *  @param writeTimeOut write timeout
     *  @param connectionTimeout connection timeout
     *  @param certFile optional TLS certificate file
     *  @param keyFile optional TLS key file */
    ClientConfiguration(std::string host, int32_t port,
                        std::chrono::milliseconds readTimeOut,
                        std::chrono::milliseconds writeTimeOut,
                        std::chrono::milliseconds connectionTimeout,
                        std::filesystem::path certFile = "",
                        std::filesystem::path keyFile = "");

    /** @return the host address */
    [[nodiscard]] const std::string &getHost() const;

    /** @return the port number */
    [[nodiscard]] int32_t getPort() const;

    /** @return the read timeout */
    [[nodiscard]] const std::chrono::milliseconds &getReadTimeOut() const;

    /** @return the write timeout */
    [[nodiscard]] const std::chrono::milliseconds &getWriteTimeOut() const;

    /** @return the connection timeout */
    [[nodiscard]] const std::chrono::milliseconds &getConnectionTimeout() const;

    /** @return the TLS certificate file path */
    [[nodiscard]] const std::filesystem::path &getCertFile() const;

    /** @return the TLS key file path */
    [[nodiscard]] const std::filesystem::path &getKeyFile() const;

    /** Print configuration to stream */
    friend std::ostream &operator<<(std::ostream &os,
                                    const ClientConfiguration &configuration);
};

#endif  // HTTP_LIBRARY_CLIENTCONFIGURATION_H
