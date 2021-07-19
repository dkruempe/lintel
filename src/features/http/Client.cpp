#include "base_library/features/http/Client.h"

std::shared_ptr<httplib::SSLClient>
Client::buildSslClient(const ClientConfiguration &clientConfiguration) {
    if (clientConfiguration.getKeyFile().empty()) { return nullptr; }
    if (clientConfiguration.getCertFile().empty()) { return nullptr; }
    return std::make_shared<httplib::SSLClient>(
            clientConfiguration.getHost().c_str(),
            clientConfiguration.getPort(),
            clientConfiguration.getCertFile().c_str(),
            clientConfiguration.getKeyFile().c_str());
}
Client::Client(const ClientConfiguration &clientConfiguration)
    : m_sslClient(buildSslClient(clientConfiguration)),
      m_client(std::make_shared<httplib::Client>(
              clientConfiguration.getHost().c_str(),
              clientConfiguration.getPort())) {
    if (clientConfiguration.getReadTimeOut().count() != 0) {
        if (isSslClient()) {
            m_sslClient->set_read_timeout(clientConfiguration.getReadTimeOut());
        } else {
            m_client->set_read_timeout(clientConfiguration.getReadTimeOut());
        }
    }

    if (clientConfiguration.getWriteTimeOut().count() != 0) {
        if (isSslClient()) {
            m_sslClient->set_write_timeout(
                    clientConfiguration.getWriteTimeOut());
        } else {
            m_client->set_write_timeout(clientConfiguration.getWriteTimeOut());
        }
    }

    if (clientConfiguration.getConnectionTimeout().count() != 0) {
        if (isSslClient()) {
            m_sslClient->set_connection_timeout(
                    clientConfiguration.getConnectionTimeout());
        } else {
            m_client->set_connection_timeout(
                    clientConfiguration.getConnectionTimeout());
        }
    }
}
[[maybe_unused]] httplib::Result Client::get(std::string_view path) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Get(pathStr.c_str()); }
    return m_sslClient->Get(pathStr.c_str());
}
[[maybe_unused]] httplib::Result Client::get(std::string_view path,
                                             const httplib::Headers &headers) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Get(pathStr.c_str(), headers); }
    return m_sslClient->Get(pathStr.c_str(), headers);
}
[[maybe_unused]] httplib::Result Client::get(std::string_view path,
                                             httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), std::move(progress));
}
[[maybe_unused]] httplib::Result Client::get(std::string_view path,
                                             const httplib::Headers &headers,
                                             httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), headers, std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), headers, std::move(progress));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, httplib::ContentReceiver contentReceiver) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr.c_str(), std::move(contentReceiver));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, const httplib::Headers &headers,
            httplib::ContentReceiver contentReceiver) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), headers,
                             std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr.c_str(), headers,
                            std::move(contentReceiver));
}
Client::~Client() {
    if (m_client != nullptr) {
        m_client->stop();
    } else {
        m_sslClient->stop();
    }
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, httplib::ContentReceiver contentReceiver,
            httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), std::move(contentReceiver),
                             std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), std::move(contentReceiver),
                            std::move(progress));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, const httplib::Headers &headers,
            httplib::ContentReceiver contentReceiver,
            httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), headers,
                             std::move(contentReceiver), std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), headers,
                            std::move(contentReceiver), std::move(progress));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, httplib::ResponseHandler responseHandler,
            httplib::ContentReceiver contentReceiver) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), std::move(responseHandler),
                             std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr.c_str(), std::move(responseHandler),
                            std::move(contentReceiver));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, const httplib::Headers &headers,
            httplib::ResponseHandler responseHandler,
            httplib::ContentReceiver contentReceiver) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), headers,
                             std::move(responseHandler),
                             std::move(contentReceiver));
    }
    return m_sslClient->Get(pathStr.c_str(), headers,
                            std::move(responseHandler),
                            std::move(contentReceiver));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, httplib::ResponseHandler responseHandler,
            httplib::ContentReceiver contentReceiver,
            httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), std::move(responseHandler),
                             std::move(contentReceiver), progress);
    }
    return m_sslClient->Get(pathStr.c_str(), std::move(responseHandler),
                            std::move(contentReceiver), std::move(progress));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, const httplib::Headers &headers,
            httplib::ResponseHandler responseHandler,
            httplib::ContentReceiver contentReceiver,
            httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), headers,
                             std::move(responseHandler),
                             std::move(contentReceiver), std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), headers,
                            std::move(responseHandler),
                            std::move(contentReceiver), progress);
}
[[maybe_unused]] httplib::Result Client::get(std::string_view path,
                                             const httplib::Params &params,
                                             const httplib::Headers &headers,
                                             httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), params, headers,
                             std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), params, headers,
                            std::move(progress));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, const httplib::Params &params,
            const httplib::Headers &headers,
            httplib::ContentReceiver contentReceiver,
            httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), params, headers,
                             std::move(contentReceiver), std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), params, headers,
                            std::move(contentReceiver), std::move(progress));
}
[[maybe_unused]] httplib::Result
Client::get(std::string_view path, const httplib::Params &params,
            const httplib::Headers &headers,
            httplib::ResponseHandler responseHandler,
            httplib::ContentReceiver contentReceiver,
            httplib::Progress progress) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Get(pathStr.c_str(), params, headers,
                             std::move(responseHandler),
                             std::move(contentReceiver), std::move(progress));
    }
    return m_sslClient->Get(pathStr.c_str(), params, headers,
                            std::move(responseHandler),
                            std::move(contentReceiver), std::move(progress));
}
bool Client::isSslClient() { return m_sslClient != nullptr; }
[[maybe_unused]] httplib::Result Client::post(std::string_view path) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Post(pathStr.c_str()); }
    return m_sslClient->Post(pathStr.c_str());
}
[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const char *body,
                                              size_t contentLength,
                                              const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), body, contentLength,
                              contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), body, contentLength, contentType);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path, const httplib::Headers &headers,
             const char *body, size_t contentLength, const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), headers, body, contentLength,
                              contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), headers, body, contentLength,
                             contentType);
}
[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const std::string &body,
                                              const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), body, contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), body, contentType);
}
[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const httplib::Headers &headers,
                                              const std::string &body,
                                              const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), headers, body, contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), headers, body, contentType);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path, size_t contentLength,
             httplib::ContentProvider contentProvider,
             const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), contentLength,
                              std::move(contentProvider), contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), contentLength,
                             std::move(contentProvider), contentType);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path,
             httplib::ContentProviderWithoutLength contentProvider,
             const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), std::move(contentProvider),
                              contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), contentType, contentType);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path, const httplib::Headers &headers,
             size_t contentLength, httplib::ContentProvider contentProvider,
             const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), headers, contentLength,
                              std::move(contentProvider), contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), headers, contentLength,
                             std::move(contentProvider), contentType);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path, const httplib::Headers &headers,
             httplib::ContentProviderWithoutLength contentProvider,
             const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), headers,
                              std::move(contentProvider), contentType);
    }
    return m_sslClient->Post(pathStr.c_str(), headers,
                             std::move(contentProvider), contentType);
}
[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const httplib::Params &params) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Post(pathStr.c_str(), params); }
    return m_sslClient->Post(pathStr.c_str(), params);
}
[[maybe_unused]] httplib::Result Client::post(std::string_view path,
                                              const httplib::Headers &headers,
                                              const httplib::Params &params) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), headers, params);
    }
    return m_sslClient->Post(pathStr.c_str(), headers, params);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path,
             const httplib::MultipartFormDataItems &items) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Post(pathStr.c_str(), items); }
    return m_sslClient->Post(pathStr.c_str(), items);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path, const httplib::Headers &headers,
             const httplib::MultipartFormDataItems &items) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), headers, items);
    }
    return m_sslClient->Post(pathStr.c_str(), headers, items);
}
[[maybe_unused]] httplib::Result
Client::post(std::string_view path, const httplib::Headers &headers,
             const httplib::MultipartFormDataItems &items,
             const std::string &boundary) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Post(pathStr.c_str(), headers, items, boundary);
    }
    return m_sslClient->Post(pathStr.c_str(), headers, items, boundary);
}
[[maybe_unused]] httplib::Result Client::put(std::string_view path) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Put(pathStr.c_str()); }
    return m_sslClient->Put(pathStr.c_str());
}
[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const char *body,
                                             size_t contentLength,
                                             const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), body, contentLength, contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), body, contentLength, contentType);
}
[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const httplib::Headers &headers,
                                             const httplib::Params &params) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), headers, params);
    }
    return m_sslClient->Put(pathStr.c_str(), headers, params);
}
[[maybe_unused]] httplib::Result
Client::put(std::string_view path, const httplib::Headers &headers,
            const char *body, size_t contentLength, const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), headers, body, contentLength,
                             contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), headers, body, contentLength,
                            contentType);
}
[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const std::string &body,
                                             const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), body, contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), body, contentType);
}
[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const httplib::Headers &headers,
                                             const std::string &body,
                                             const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), headers, body, contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), headers, body, contentType);
}
[[maybe_unused]] httplib::Result
Client::put(std::string_view path, size_t contentLength,
            httplib::ContentProvider contentProvider, const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), contentLength,
                             std::move(contentProvider), contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), contentLength,
                            std::move(contentProvider), contentType);
}
[[maybe_unused]] httplib::Result
Client::put(std::string_view path,
            httplib::ContentProviderWithoutLength contentProvider,
            const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), std::move(contentProvider),
                             contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), std::move(contentProvider),
                            contentType);
}
[[maybe_unused]] httplib::Result
Client::put(std::string_view path, const httplib::Headers &headers,
            size_t contentLength, httplib::ContentProvider contentProvider,
            const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), headers, contentLength,
                             std::move(contentProvider), contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), headers, contentLength,
                            std::move(contentProvider), contentType);
}
[[maybe_unused]] httplib::Result
Client::put(std::string_view path, const httplib::Headers &headers,
            httplib::ContentProviderWithoutLength contentProvider,
            const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Put(pathStr.c_str(), headers,
                             std::move(contentProvider), contentType);
    }
    return m_sslClient->Put(pathStr.c_str(), headers,
                            std::move(contentProvider), contentType);
}
[[maybe_unused]] httplib::Result Client::put(std::string_view path,
                                             const httplib::Params &params) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Put(pathStr.c_str(), params); }
    return m_sslClient->Put(pathStr.c_str(), params);
}
[[maybe_unused]] httplib::Result Client::deletes(std::string_view path) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) { return m_client->Delete(pathStr.c_str()); }
    return m_sslClient->Delete(pathStr.c_str());
}
[[maybe_unused]] httplib::Result
Client::deletes(std::string_view path, const httplib::Headers &headers) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr.c_str(), headers);
    }
    return m_sslClient->Delete(pathStr.c_str(), headers);
}
[[maybe_unused]] httplib::Result Client::deletes(std::string_view path,
                                                 const char *body,
                                                 size_t contentLength,
                                                 const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr.c_str(), body, contentLength,
                                contentType);
    }
    return m_sslClient->Delete(pathStr.c_str(), body, contentLength,
                               contentType);
}
[[maybe_unused]] httplib::Result
Client::deletes(std::string_view path, const httplib::Headers &headers,
                const char *body, size_t contentLength,
                const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr.c_str(), headers, body, contentLength,
                                contentType);
    }
    return m_sslClient->Delete(pathStr.c_str(), headers, body, contentLength,
                               contentType);
}
[[maybe_unused]] httplib::Result Client::deletes(std::string_view path,
                                                 const std::string &body,
                                                 const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr.c_str(), body, contentType);
    }
    return m_sslClient->Delete(pathStr.c_str(), body, contentType);
}
[[maybe_unused]] httplib::Result
Client::deletes(std::string_view path, const httplib::Headers &headers,
                const std::string &body, const char *contentType) {
    auto pathStr = std::string(path);
    if (m_client != nullptr) {
        return m_client->Delete(pathStr.c_str(), headers, body, contentType);
    }
    return m_sslClient->Delete(pathStr.c_str(), headers, body, contentType);
}
