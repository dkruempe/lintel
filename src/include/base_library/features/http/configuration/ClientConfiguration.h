#ifndef HTTP_LIBRARY_CLIENTCONFIGURATION_H
#define HTTP_LIBRARY_CLIENTCONFIGURATION_H

#include <chrono>
#include <filesystem>
#include <ostream>
#include <string>

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
    ClientConfiguration(std::string host, int32_t port,
                        std::chrono::milliseconds readTimeOut,
                        std::chrono::milliseconds writeTimeOut,
                        std::chrono::milliseconds connectionTimeout,
                        std::filesystem::path certFile = "",
                        std::filesystem::path keyFile = "");

    [[nodiscard]] const std::string &getHost() const;

    [[nodiscard]] int32_t getPort() const;

    [[nodiscard]] const std::chrono::milliseconds &getReadTimeOut() const;

    [[nodiscard]] const std::chrono::milliseconds &getWriteTimeOut() const;

    [[nodiscard]] const std::chrono::milliseconds &getConnectionTimeout() const;

    [[nodiscard]] const std::filesystem::path &getCertFile() const;

    [[nodiscard]] const std::filesystem::path &getKeyFile() const;

    friend std::ostream &operator<<(std::ostream &os,
                                    const ClientConfiguration &configuration);
};

#endif  // HTTP_LIBRARY_CLIENTCONFIGURATION_H
