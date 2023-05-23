#ifndef HTTP_LIBRARY_SERVERCONFIGURATION_H
#define HTTP_LIBRARY_SERVERCONFIGURATION_H

#include <chrono>
#include <filesystem>
#include <ostream>
#include <string>

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
    ServerConfiguration(std::string host, int32_t port,
                        const std::chrono::milliseconds &readTimeOut =
                        std::chrono::milliseconds(0),
                        const std::chrono::milliseconds &writeTimeOut =
                        std::chrono::milliseconds(0),
                        const std::chrono::milliseconds &idleInterval =
                        std::chrono::milliseconds(0),
                        std::filesystem::path certFile = "",
                        std::filesystem::path keyFile = "");

    [[nodiscard]] const std::string &getHost() const;

    [[nodiscard]] int32_t getPort() const;

    [[nodiscard]] const std::chrono::milliseconds &getReadTimeOut() const;

    [[nodiscard]] const std::chrono::milliseconds &getWriteTimeOut() const;

    [[nodiscard]] const std::chrono::milliseconds &getIdleInterval() const;

    [[nodiscard]] const std::filesystem::path &getCertFile() const;

    [[nodiscard]] const std::filesystem::path &getKeyFile() const;

    friend std::ostream &operator<<(std::ostream &os,
                                    const ServerConfiguration &configuration);
};

#endif  // HTTP_LIBRARY_SERVERCONFIGURATION_H
