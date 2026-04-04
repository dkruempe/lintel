#ifndef HTTP_LIBRARY_CLIENT_H
#define HTTP_LIBRARY_CLIENT_H

#include <httplib.h>

#include <filesystem>
#include <string>

#include "base_library/features/http/configuration/ClientConfiguration.h"

class Client {
private:
    std::shared_ptr<httplib::SSLClient> m_sslClient;
    std::shared_ptr<httplib::Client> m_client;

    static std::shared_ptr<httplib::SSLClient> buildSslClient(
            const std::shared_ptr<ClientConfiguration> &clientConfiguration);

    bool isSslClient() const;

public:
    explicit Client(const std::shared_ptr<ClientConfiguration> &clientConfiguration);

    ~Client();

    [[maybe_unused]] void setBasicAuth(const std::string &userName,
                                       const std::string &password) const;

    [[maybe_unused]] void setDigestAuth(const std::string &userName,
                                        const std::string &password) const;

    [[maybe_unused]] void setBearerTokenAuth(const std::string &token) const;

    [[maybe_unused]] httplib::Result get(std::string_view path) const;

    [[maybe_unused]] httplib::Result get(std::string_view path,
                                         const httplib::Headers &headers) const;

    [[maybe_unused]] httplib::Result get(
            std::string_view path, httplib::ContentReceiver contentReceiver) const;

    [[maybe_unused]] httplib::Result get(
            std::string_view path, const httplib::Headers &headers,
            httplib::ContentReceiver contentReceiver) const;

    [[maybe_unused]] httplib::Result get(
            std::string_view path, httplib::ResponseHandler responseHandler,
            httplib::ContentReceiver contentReceiver) const;

    [[maybe_unused]] httplib::Result get(
            std::string_view path, const httplib::Headers &headers,
            httplib::ResponseHandler responseHandler,
            httplib::ContentReceiver contentReceiver) const;

    [[maybe_unused]] httplib::Result post(std::string_view path) const;

    [[maybe_unused]] httplib::Result post(std::string_view path, const char *body,
                                          size_t contentLength,
                                          const char *contentType) const;

    [[maybe_unused]] httplib::Result post(std::string_view path,
                                          const httplib::Headers &headers,
                                          const char *body, size_t contentLength,
                                          const char *contentType) const;

    [[maybe_unused]] httplib::Result post(std::string_view path,
                                          const std::string &body,
                                          const char *contentType) const;

    [[maybe_unused]] httplib::Result post(std::string_view path,
                                          const httplib::Headers &headers,
                                          const std::string &body,
                                          const char *contentType) const;

    [[maybe_unused]] httplib::Result post(
            std::string_view path, size_t contentLength,
            httplib::ContentProvider contentProvider, const char *contentType) const;

    [[maybe_unused]] httplib::Result post(
            std::string_view path,
            httplib::ContentProviderWithoutLength contentProvider,
            const char *contentType) const;

    [[maybe_unused]] httplib::Result post(
            std::string_view path, const httplib::Headers &headers,
            size_t contentLength, httplib::ContentProvider contentProvider,
            const char *contentType) const;

    [[maybe_unused]] httplib::Result post(
            std::string_view path, const httplib::Headers &headers,
            httplib::ContentProviderWithoutLength contentProvider,
            const char *contentType) const;

    [[maybe_unused]] httplib::Result post(std::string_view path,
                                          const httplib::Params &params) const;

    [[maybe_unused]] httplib::Result post(std::string_view path,
                                          const httplib::Headers &headers,
                                          const httplib::Params &params) const;

    [[maybe_unused]] httplib::Result put(std::string_view path) const;

    [[maybe_unused]] httplib::Result put(std::string_view path, const char *body,
                                         size_t contentLength,
                                         const char *contentType) const;

    [[maybe_unused]] httplib::Result put(std::string_view path,
                                         const httplib::Headers &headers,
                                         const char *body, size_t contentLength,
                                         const char *contentType) const;

    [[maybe_unused]] httplib::Result put(std::string_view path,
                                         const std::string &body,
                                         const char *contentType) const;

    [[maybe_unused]] httplib::Result put(std::string_view path,
                                         const httplib::Headers &headers,
                                         const std::string &body,
                                         const char *contentType) const;

    [[maybe_unused]] httplib::Result put(std::string_view path,
                                         size_t contentLength,
                                         httplib::ContentProvider contentProvider,
                                         const char *contentType) const;

    [[maybe_unused]] httplib::Result put(
            std::string_view path,
            httplib::ContentProviderWithoutLength contentProvider,
            const char *contentType) const;

    [[maybe_unused]] httplib::Result put(std::string_view path,
                                         const httplib::Headers &headers,
                                         size_t contentLength,
                                         httplib::ContentProvider contentProvider,
                                         const char *contentType) const;

    [[maybe_unused]] httplib::Result put(
            std::string_view path, const httplib::Headers &headers,
            httplib::ContentProviderWithoutLength contentProvider,
            const char *contentType) const;

    [[maybe_unused]] httplib::Result put(std::string_view path,
                                         const httplib::Params &params) const;

    [[maybe_unused]] httplib::Result put(std::string_view path,
                                         const httplib::Headers &headers,
                                         const httplib::Params &params) const;

    [[maybe_unused]] httplib::Result deletes(std::string_view path) const;

    [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                             const httplib::Headers &headers) const;

    [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                             const char *body,
                                             size_t contentLength,
                                             const char *contentType) const;

    [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                             const httplib::Headers &headers,
                                             const char *body,
                                             size_t contentLength,
                                             const char *contentType) const;

    [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                             const std::string &body,
                                             const char *contentType) const;

    [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                             const httplib::Headers &headers,
                                             const std::string &body,
                                             const char *contentType) const;
};

#endif  // HTTP_LIBRARY_CLIENT_H
