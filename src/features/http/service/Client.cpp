#include "base_library/features/http/service/Client.h"

std::shared_ptr<httplib::SSLClient> Client::buildSslClient(
        const std::shared_ptr<ClientConfiguration> &clientConfiguration) {
    if (clientConfiguration->getKeyFile().empty()) {
        return nullptr;
    }
    if (clientConfiguration->getCertFile().empty()) {
        return nullptr;
    }
    std::shared_ptr<httplib::SSLClient> sslClient =
            std::make_shared<httplib::SSLClient>(
                    clientConfiguration->getHost(),
                    clientConfiguration->getPort(),
                    clientConfiguration->getCertFile(),
                    clientConfiguration->getKeyFile());
    if (!clientConfiguration->getCaCertFile().empty()) {
        sslClient->set_ca_cert_path(
                clientConfiguration->getCaCertFile().string());
    }
    return sslClient;
}

Client::Client(const std::shared_ptr<ClientConfiguration> &clientConfiguration)
        : m_sslClient(buildSslClient(clientConfiguration)),
          m_client(m_sslClient == nullptr
                   ? std::make_shared<httplib::Client>(
                           clientConfiguration->getHost(),
                           clientConfiguration->getPort())
                   : nullptr) {
    if (clientConfiguration->getReadTimeOut().count() != 0) {
        if (isSslClient()) {
            m_sslClient->set_read_timeout(clientConfiguration->getReadTimeOut());
        } else {
            m_client->set_read_timeout(clientConfiguration->getReadTimeOut());
        }
    }

    if (clientConfiguration->getWriteTimeOut().count() != 0) {
        if (isSslClient()) {
            m_sslClient->set_write_timeout(clientConfiguration->getWriteTimeOut());
        } else {
            m_client->set_write_timeout(clientConfiguration->getWriteTimeOut());
        }
    }

    if (clientConfiguration->getConnectionTimeout().count() != 0) {
        if (isSslClient()) {
            m_sslClient->set_connection_timeout(
                    clientConfiguration->getConnectionTimeout());
        } else {
            m_client->set_connection_timeout(
                    clientConfiguration->getConnectionTimeout());
        }
    }

    auto errorLogger = [](const httplib::Error &error,
                          const httplib::Request *) {
        LOG_ERROR("httplib client error: {}", httplib::to_string(error));
    };
    if (isSslClient()) {
        m_sslClient->set_error_logger(errorLogger);
    } else {
        m_client->set_error_logger(errorLogger);
    }
}

[[maybe_unused]] httplib::Result Client::get(std::string_view path) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr);
    }
    return m_sslClient->Get(pathStr);
}

[[maybe_unused]] httplib::Result Client::get(std::string_view path,
                                             const httplib::Headers &headers) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr, headers);
    }
    return m_sslClient->Get(pathStr, headers);
}

[[maybe_unused]] httplib::Result Client::get(
        std::string_view path, httplib::ContentReceiver contentReceiver) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr, std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr, std::move(contentReceiver));
}

[[maybe_unused]] httplib::Result Client::get(
        std::string_view path, const httplib::Headers &headers,
        httplib::ContentReceiver contentReceiver) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr, headers, std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr, headers, std::move(contentReceiver));
}

Client::~Client() {
    if (m_client != nullptr) {
        m_client->stop();
    } else {
        m_sslClient->stop();
    }
}

[[maybe_unused]] httplib::Result Client::get(
        std::string_view path, httplib::ResponseHandler responseHandler,
        httplib::ContentReceiver contentReceiver) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr, std::move(responseHandler),
                             std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr, std::move(responseHandler),
                            std::move(contentReceiver));
}

[[maybe_unused]] httplib::Result Client::get(
        std::string_view path, const httplib::Headers &headers,
        httplib::ResponseHandler responseHandler,
        httplib::ContentReceiver contentReceiver) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr, headers, std::move(responseHandler),
                             std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr, headers, std::move(responseHandler),
                            std::move(contentReceiver));
}

bool Client::isSslClient() const { return m_sslClient != nullptr; }

[[maybe_unused]] httplib::Result Client::post(std::string_view path) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr);
    }
    return m_sslClient->Post(pathStr);
}

[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const char *body,
                                              size_t contentLength,
                                              const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, body, contentLength, contentType);
    }
    return m_sslClient->Post(pathStr, body, contentLength, contentType);
}

[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const httplib::Headers &headers,
                                              const char *body,
                                              size_t contentLength,
                                              const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, headers, body, contentLength,
                              contentType);
    }
    return m_sslClient->Post(pathStr, headers, body, contentLength,
                             contentType);
}

[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const std::string &body,
                                              const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, body, contentType);
    }
    return m_sslClient->Post(pathStr, body, contentType);
}

[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const httplib::Headers &headers,
                                              const std::string &body,
                                              const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, headers, body, contentType);
    }
    return m_sslClient->Post(pathStr, headers, body, contentType);
}

[[maybe_unused]] httplib::Result Client::post(
        std::string_view path, size_t contentLength,
        httplib::ContentProvider contentProvider, const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, contentLength,
                              std::move(contentProvider), contentType);
    }
    return m_sslClient->Post(pathStr, contentLength,
                             std::move(contentProvider), contentType);
}

[[maybe_unused]] httplib::Result Client::post(
        std::string_view path,
        httplib::ContentProviderWithoutLength contentProvider,
        const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, std::move(contentProvider),
                              contentType);
    }
    return m_sslClient->Post(pathStr, std::move(contentProvider), contentType);
}

[[maybe_unused]] httplib::Result Client::post(
        std::string_view path, const httplib::Headers &headers,
        size_t contentLength, httplib::ContentProvider contentProvider,
        const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, headers, contentLength,
                              std::move(contentProvider), contentType);
    }
    return m_sslClient->Post(pathStr, headers, contentLength,
                             std::move(contentProvider), contentType);
}

[[maybe_unused]] httplib::Result Client::post(
        std::string_view path, const httplib::Headers &headers,
        httplib::ContentProviderWithoutLength contentProvider,
        const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, headers, std::move(contentProvider),
                              contentType);
    }
    return m_sslClient->Post(pathStr, headers, std::move(contentProvider),
                             contentType);
}

[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const httplib::Params &params) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, params);
    }
    return m_sslClient->Post(pathStr, params);
}

[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const httplib::Headers &headers,
                                              const httplib::Params &params) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr, headers, params);
    }
    return m_sslClient->Post(pathStr, headers, params);
}

[[maybe_unused]] httplib::Result Client::put(std::string_view path) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr);
    }
    return m_sslClient->Put(pathStr);
}

[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const char *body,
                                             size_t contentLength,
                                             const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, body, contentLength, contentType);
    }
    return m_sslClient->Put(pathStr, body, contentLength, contentType);
}

[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const httplib::Headers &headers,
                                             const httplib::Params &params) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, headers, params);
    }
    return m_sslClient->Put(pathStr, headers, params);
}

[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const httplib::Headers &headers,
                                             const char *body,
                                             size_t contentLength,
                                             const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, headers, body, contentLength,
                             contentType);
    }
    return m_sslClient->Put(pathStr, headers, body, contentLength,
                            contentType);
}

[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const std::string &body,
                                             const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, body, contentType);
    }
    return m_sslClient->Put(pathStr, body, contentType);
}

[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const httplib::Headers &headers,
                                             const std::string &body,
                                             const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, headers, body, contentType);
    }
    return m_sslClient->Put(pathStr, headers, body, contentType);
}

[[maybe_unused]] httplib::Result Client::put(
        std::string_view path, size_t contentLength,
        httplib::ContentProvider contentProvider, const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, contentLength,
                             std::move(contentProvider), contentType);
    }
    return m_sslClient->Put(pathStr, contentLength,
                            std::move(contentProvider), contentType);
}

[[maybe_unused]] httplib::Result Client::put(
        std::string_view path,
        httplib::ContentProviderWithoutLength contentProvider,
        const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, std::move(contentProvider),
                             contentType);
    }
    return m_sslClient->Put(pathStr, std::move(contentProvider),
                            contentType);
}

[[maybe_unused]] httplib::Result Client::put(
        std::string_view path, const httplib::Headers &headers,
        size_t contentLength, httplib::ContentProvider contentProvider,
        const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, headers, contentLength,
                             std::move(contentProvider), contentType);
    }
    return m_sslClient->Put(pathStr, headers, contentLength,
                            std::move(contentProvider), contentType);
}

[[maybe_unused]] httplib::Result Client::put(
        std::string_view path, const httplib::Headers &headers,
        httplib::ContentProviderWithoutLength contentProvider,
        const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, headers, std::move(contentProvider),
                             contentType);
    }
    return m_sslClient->Put(pathStr, headers, std::move(contentProvider),
                            contentType);
}

[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const httplib::Params &params) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr, params);
    }
    return m_sslClient->Put(pathStr, params);
}

[[maybe_unused]] httplib::Result Client::deletes(std::string_view path) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr);
    }
    return m_sslClient->Delete(pathStr);
}

[[maybe_unused]] httplib::Result Client::deletes(
        std::string_view path, const httplib::Headers &headers) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr, headers);
    }
    return m_sslClient->Delete(pathStr, headers);
}

[[maybe_unused]] httplib::Result Client::deletes(std::string_view path,
                                                 const char *body,
                                                 size_t contentLength,
                                                 const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr, body, contentLength, contentType);
    }
    return m_sslClient->Delete(pathStr, body, contentLength, contentType);
}

[[maybe_unused]] httplib::Result Client::deletes(
        std::string_view path, const httplib::Headers &headers, const char *body,
        size_t contentLength, const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr, headers, body, contentLength,
                                contentType);
    }
    return m_sslClient->Delete(pathStr, headers, body, contentLength,
                               contentType);
}

[[maybe_unused]] httplib::Result Client::deletes(std::string_view path,
                                                 const std::string &body,
                                                 const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr, body, contentType);
    }
    return m_sslClient->Delete(pathStr, body, contentType);
}

[[maybe_unused]] httplib::Result Client::deletes(
        std::string_view path, const httplib::Headers &headers,
        const std::string &body, const char *contentType) const
{
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr, headers, body, contentType);
    }
    return m_sslClient->Delete(pathStr, headers, body, contentType);
}

void Client::setBasicAuth(const std::string &userName,
                          const std::string &password) const
{
    if (m_client != nullptr) {
        m_client->set_basic_auth(userName, password);
        return;
    }
    if (m_sslClient != nullptr) {
        m_sslClient->set_basic_auth(userName, password);
    }
}

void Client::setDigestAuth(const std::string &userName,
                           const std::string &password) const
{
    if (m_client != nullptr) {
        m_client->set_digest_auth(userName, password);
        return;
    }
    if (m_sslClient != nullptr) {
        m_sslClient->set_digest_auth(userName, password);
    }
}

void Client::setBearerTokenAuth(const std::string &token) const
{
    if (m_client != nullptr) {
        m_client->set_bearer_token_auth(token);
        return;
    }
    if (m_sslClient != nullptr) {
        m_sslClient->set_bearer_token_auth(token);
    }
}
