#ifndef HTTP_LIBRARY_SERVERCONFIGURATION_H
#define HTTP_LIBRARY_SERVERCONFIGURATION_H

#include <chrono>
#include <filesystem>
#include <ostream>
#include <string>

/** Configuration for an HTTP server (host, port, timeouts, TLS) */
class ServerConfiguration {
private:
    const std::string m_host;
    const int32_t m_port;
    std::chrono::milliseconds m_readTimeOut;
    std::chrono::milliseconds m_writeTimeOut;
    std::chrono::milliseconds m_idleInterval;
    std::filesystem::path m_certFile;
    std::filesystem::path m_keyFile;

public:
    /** @param host bind address
     *  @param port listen port
     *  @param readTimeOut read timeout (default 0 = no timeout)
     *  @param writeTimeOut write timeout
     *  @param idleInterval idle connection timeout
     *  @param certFile TLS certificate file path
     *  @param keyFile TLS key file path */
    ServerConfiguration(std::string host, int32_t port,
                        const std::chrono::milliseconds &readTimeOut =
                        std::chrono::milliseconds(0),
                        const std::chrono::milliseconds &writeTimeOut =
                        std::chrono::milliseconds(0),
                        const std::chrono::milliseconds &idleInterval =
                        std::chrono::milliseconds(0),
                        std::filesystem::path certFile = "",
                        std::filesystem::path keyFile = "");

    /** @return the bind host */
    [[nodiscard]] const std::string &getHost() const;

    /** @return the listen port */
    [[nodiscard]] int32_t getPort() const;

    /** @return the read timeout */
    [[nodiscard]] const std::chrono::milliseconds &getReadTimeOut() const;

    /** @return the write timeout */
    [[nodiscard]] const std::chrono::milliseconds &getWriteTimeOut() const;

    /** @return the idle interval */
    [[nodiscard]] const std::chrono::milliseconds &getIdleInterval() const;

    /** @return the TLS certificate file path */
    [[nodiscard]] const std::filesystem::path &getCertFile() const;

    /** @return the TLS key file path */
    [[nodiscard]] const std::filesystem::path &getKeyFile() const;

    /** Print configuration to stream */
    friend std::ostream &operator<<(std::ostream &os,
                                    const ServerConfiguration &configuration);
};

#endif  // HTTP_LIBRARY_SERVERCONFIGURATION_H
