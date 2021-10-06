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
      const ClientConfiguration &clientConfiguration);

  bool isSslClient();

 public:
  explicit Client(const ClientConfiguration &clientConfiguration);

  ~Client();

  [[maybe_unused]] void setBasicAuth(const std::string &userName,
                                     const std::string &password);

  [[maybe_unused]] void setDigestAuth(const std::string &userName,
                                      const std::string &password);

  [[maybe_unused]] void setBearerTokenAuth(const std::string &token);

  [[maybe_unused]] httplib::Result get(std::string_view path);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       const httplib::Headers &headers);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       httplib::Progress progress);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       const httplib::Headers &headers,
                                       httplib::Progress progress);
  [[maybe_unused]] httplib::Result get(
      std::string_view path, httplib::ContentReceiver contentReceiver);
  [[maybe_unused]] httplib::Result get(
      std::string_view path, const httplib::Headers &headers,
      httplib::ContentReceiver contentReceiver);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       httplib::ContentReceiver contentReceiver,
                                       httplib::Progress progress);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       const httplib::Headers &headers,
                                       httplib::ContentReceiver contentReceiver,
                                       httplib::Progress progress);
  [[maybe_unused]] httplib::Result get(
      std::string_view path, httplib::ResponseHandler responseHandler,
      httplib::ContentReceiver contentReceiver);
  [[maybe_unused]] httplib::Result get(
      std::string_view path, const httplib::Headers &headers,
      httplib::ResponseHandler responseHandler,
      httplib::ContentReceiver contentReceiver);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       httplib::ResponseHandler responseHandler,
                                       httplib::ContentReceiver contentReceiver,
                                       httplib::Progress progress);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       const httplib::Headers &headers,
                                       httplib::ResponseHandler responseHandler,
                                       httplib::ContentReceiver contentReceiver,
                                       httplib::Progress progress);

  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       const httplib::Params &params,
                                       const httplib::Headers &headers,
                                       httplib::Progress progress = nullptr);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       const httplib::Params &params,
                                       const httplib::Headers &headers,
                                       httplib::ContentReceiver contentReceiver,
                                       httplib::Progress progress = nullptr);
  [[maybe_unused]] httplib::Result get(std::string_view path,
                                       const httplib::Params &params,
                                       const httplib::Headers &headers,
                                       httplib::ResponseHandler responseHandler,
                                       httplib::ContentReceiver contentReceiver,
                                       httplib::Progress progress = nullptr);
  [[maybe_unused]] httplib::Result post(std::string_view path);
  [[maybe_unused]] httplib::Result post(std::string_view path, const char *body,
                                        size_t contentLength,
                                        const char *contentType);
  [[maybe_unused]] httplib::Result post(std::string_view path,
                                        const httplib::Headers &headers,
                                        const char *body, size_t contentLength,
                                        const char *contentType);
  [[maybe_unused]] httplib::Result post(std::string_view path,
                                        const std::string &body,
                                        const char *contentType);
  [[maybe_unused]] httplib::Result post(std::string_view path,
                                        const httplib::Headers &headers,
                                        const std::string &body,
                                        const char *contentType);
  [[maybe_unused]] httplib::Result post(
      std::string_view path, size_t contentLength,
      httplib::ContentProvider contentProvider, const char *contentType);
  [[maybe_unused]] httplib::Result post(
      std::string_view path,
      httplib::ContentProviderWithoutLength contentProvider,
      const char *contentType);
  [[maybe_unused]] httplib::Result post(
      std::string_view path, const httplib::Headers &headers,
      size_t contentLength, httplib::ContentProvider contentProvider,
      const char *contentType);
  [[maybe_unused]] httplib::Result post(
      std::string_view path, const httplib::Headers &headers,
      httplib::ContentProviderWithoutLength contentProvider,
      const char *contentType);
  [[maybe_unused]] httplib::Result post(std::string_view path,
                                        const httplib::Params &params);
  [[maybe_unused]] httplib::Result post(std::string_view path,
                                        const httplib::Headers &headers,
                                        const httplib::Params &params);
  [[maybe_unused]] httplib::Result post(
      std::string_view path, const httplib::MultipartFormDataItems &items);
  [[maybe_unused]] httplib::Result post(
      std::string_view path, const httplib::Headers &headers,
      const httplib::MultipartFormDataItems &items);
  [[maybe_unused]] httplib::Result post(
      std::string_view path, const httplib::Headers &headers,
      const httplib::MultipartFormDataItems &items,
      const std::string &boundary);
  [[maybe_unused]] httplib::Result put(std::string_view path);
  [[maybe_unused]] httplib::Result put(std::string_view path, const char *body,
                                       size_t contentLength,
                                       const char *contentType);
  [[maybe_unused]] httplib::Result put(std::string_view path,
                                       const httplib::Headers &headers,
                                       const char *body, size_t contentLength,
                                       const char *contentType);
  [[maybe_unused]] httplib::Result put(std::string_view path,
                                       const std::string &body,
                                       const char *contentType);
  [[maybe_unused]] httplib::Result put(std::string_view path,
                                       const httplib::Headers &headers,
                                       const std::string &body,
                                       const char *contentType);
  [[maybe_unused]] httplib::Result put(std::string_view path,
                                       size_t contentLength,
                                       httplib::ContentProvider contentProvider,
                                       const char *contentType);
  [[maybe_unused]] httplib::Result put(
      std::string_view path,
      httplib::ContentProviderWithoutLength contentProvider,
      const char *contentType);
  [[maybe_unused]] httplib::Result put(std::string_view path,
                                       const httplib::Headers &headers,
                                       size_t contentLength,
                                       httplib::ContentProvider contentProvider,
                                       const char *contentType);
  [[maybe_unused]] httplib::Result put(
      std::string_view path, const httplib::Headers &headers,
      httplib::ContentProviderWithoutLength contentProvider,
      const char *contentType);
  [[maybe_unused]] httplib::Result put(std::string_view path,
                                       const httplib::Params &params);
  [[maybe_unused]] httplib::Result put(std::string_view path,
                                       const httplib::Headers &headers,
                                       const httplib::Params &params);
  [[maybe_unused]] httplib::Result deletes(std::string_view path);
  [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                           const httplib::Headers &headers);
  [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                           const char *body,
                                           size_t contentLength,
                                           const char *contentType);
  [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                           const httplib::Headers &headers,
                                           const char *body,
                                           size_t contentLength,
                                           const char *contentType);
  [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                           const std::string &body,
                                           const char *contentType);
  [[maybe_unused]] httplib::Result deletes(std::string_view path,
                                           const httplib::Headers &headers,
                                           const std::string &body,
                                           const char *contentType);
};

#endif  // HTTP_LIBRARY_CLIENT_H
