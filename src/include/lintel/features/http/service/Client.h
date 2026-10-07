#ifndef HTTP_LIBRARY_CLIENT_H
#define HTTP_LIBRARY_CLIENT_H

#include "lintel/features/http/configuration/ClientConfiguration.h"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

/**
 * Forward declarations mirroring the subset of <httplib.h> that is part of the
 * public API. Keep this block in sync with the vendored httplib version so that
 * the aliases resolve to the exact same types when <httplib.h> is included.
 */
namespace httplib {

class Result;
struct Response;
class DataSink;

namespace detail {
  namespace case_ignore {
    struct equal_to;
    struct hash;
  }// namespace case_ignore
}// namespace detail

using Headers =
  std::unordered_multimap<std::string, std::string, detail::case_ignore::hash, detail::case_ignore::equal_to>;
using Params = std::multimap<std::string, std::string>;
using ResponseHandler = std::function<bool(const Response &response)>;
using ContentProvider = std::function<bool(size_t offset, size_t length, DataSink &sink)>;
using ContentProviderWithoutLength = std::function<bool(size_t offset, DataSink &sink)>;
using ContentReceiver = std::function<bool(const char *data, size_t data_length)>;

}// namespace httplib

/** HTTP client wrapper supporting SSL, authentication, and common HTTP methods */
class Client
{
private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;

public:
  explicit Client(const std::shared_ptr<ClientConfiguration> &clientConfiguration);

  ~Client();

  /** Set HTTP basic authentication header */
  [[maybe_unused]] void setBasicAuth(const std::string &userName, const std::string &password) const;

  /** Set HTTP digest authentication header */
  [[maybe_unused]] void setDigestAuth(const std::string &userName, const std::string &password) const;

  /** Set HTTP Bearer token authentication header */
  [[maybe_unused]] void setBearerTokenAuth(const std::string &token) const;

  /** Perform a GET request */
  [[maybe_unused]] httplib::Result get(std::string_view path) const;

  /** Perform a GET request with headers */
  [[maybe_unused]] httplib::Result get(std::string_view path, const httplib::Headers &headers) const;

  /** Perform a GET request with a content receiver callback */
  [[maybe_unused]] httplib::Result get(std::string_view path, httplib::ContentReceiver contentReceiver) const;

  /** Perform a GET request with headers and content receiver */
  [[maybe_unused]] httplib::Result
    get(std::string_view path, const httplib::Headers &headers, httplib::ContentReceiver contentReceiver) const;

  /** Perform a GET request with response handler and content receiver */
  [[maybe_unused]] httplib::Result get(std::string_view path,
    httplib::ResponseHandler responseHandler,
    httplib::ContentReceiver contentReceiver) const;

  /** Perform a GET request with headers, response handler, and content receiver */
  [[maybe_unused]] httplib::Result get(std::string_view path,
    const httplib::Headers &headers,
    httplib::ResponseHandler responseHandler,
    httplib::ContentReceiver contentReceiver) const;

  /** Perform a POST request */
  [[maybe_unused]] httplib::Result post(std::string_view path) const;

  /** Perform a POST request with body */
  [[maybe_unused]] httplib::Result
    post(std::string_view path, const char *body, size_t contentLength, const char *contentType) const;

  /** Perform a POST request with headers and body */
  [[maybe_unused]] httplib::Result post(std::string_view path,
    const httplib::Headers &headers,
    const char *body,
    size_t contentLength,
    const char *contentType) const;

  /** Perform a POST request with string body */
  [[maybe_unused]] httplib::Result post(std::string_view path, const std::string &body, const char *contentType) const;

  /** Perform a POST request with headers and string body */
  [[maybe_unused]] httplib::Result post(std::string_view path,
    const httplib::Headers &headers,
    const std::string &body,
    const char *contentType) const;

  /** Perform a POST request with content provider */
  [[maybe_unused]] httplib::Result post(std::string_view path,
    size_t contentLength,
    httplib::ContentProvider contentProvider,
    const char *contentType) const;

  /** Perform a POST request with content provider without length */
  [[maybe_unused]] httplib::Result
    post(std::string_view path, httplib::ContentProviderWithoutLength contentProvider, const char *contentType) const;

  /** Perform a POST request with headers and content provider */
  [[maybe_unused]] httplib::Result post(std::string_view path,
    const httplib::Headers &headers,
    size_t contentLength,
    httplib::ContentProvider contentProvider,
    const char *contentType) const;

  /** Perform a POST request with headers and content provider without length */
  [[maybe_unused]] httplib::Result post(std::string_view path,
    const httplib::Headers &headers,
    httplib::ContentProviderWithoutLength contentProvider,
    const char *contentType) const;

  /** Perform a POST request with form params */
  [[maybe_unused]] httplib::Result post(std::string_view path, const httplib::Params &params) const;

  /** Perform a POST request with headers and form params */
  [[maybe_unused]] httplib::Result
    post(std::string_view path, const httplib::Headers &headers, const httplib::Params &params) const;

  /** Perform a PUT request */
  [[maybe_unused]] httplib::Result put(std::string_view path) const;

  /** Perform a PUT request with body */
  [[maybe_unused]] httplib::Result
    put(std::string_view path, const char *body, size_t contentLength, const char *contentType) const;

  /** Perform a PUT request with headers and body */
  [[maybe_unused]] httplib::Result put(std::string_view path,
    const httplib::Headers &headers,
    const char *body,
    size_t contentLength,
    const char *contentType) const;

  /** Perform a PUT request with string body */
  [[maybe_unused]] httplib::Result put(std::string_view path, const std::string &body, const char *contentType) const;

  /** Perform a PUT request with headers and string body */
  [[maybe_unused]] httplib::Result
    put(std::string_view path, const httplib::Headers &headers, const std::string &body, const char *contentType) const;

  /** Perform a PUT request with content provider */
  [[maybe_unused]] httplib::Result put(std::string_view path,
    size_t contentLength,
    httplib::ContentProvider contentProvider,
    const char *contentType) const;

  /** Perform a PUT request with content provider without length */
  [[maybe_unused]] httplib::Result
    put(std::string_view path, httplib::ContentProviderWithoutLength contentProvider, const char *contentType) const;

  /** Perform a PUT request with headers and content provider */
  [[maybe_unused]] httplib::Result put(std::string_view path,
    const httplib::Headers &headers,
    size_t contentLength,
    httplib::ContentProvider contentProvider,
    const char *contentType) const;

  /** Perform a PUT request with headers and content provider without length */
  [[maybe_unused]] httplib::Result put(std::string_view path,
    const httplib::Headers &headers,
    httplib::ContentProviderWithoutLength contentProvider,
    const char *contentType) const;

  /** Perform a PUT request with form params */
  [[maybe_unused]] httplib::Result put(std::string_view path, const httplib::Params &params) const;

  /** Perform a PUT request with headers and form params */
  [[maybe_unused]] httplib::Result
    put(std::string_view path, const httplib::Headers &headers, const httplib::Params &params) const;

  /** Perform a DELETE request */
  [[maybe_unused]] httplib::Result deletes(std::string_view path) const;

  /** Perform a DELETE request with headers */
  [[maybe_unused]] httplib::Result deletes(std::string_view path, const httplib::Headers &headers) const;

  /** Perform a DELETE request with body */
  [[maybe_unused]] httplib::Result
    deletes(std::string_view path, const char *body, size_t contentLength, const char *contentType) const;

  /** Perform a DELETE request with headers and body */
  [[maybe_unused]] httplib::Result deletes(std::string_view path,
    const httplib::Headers &headers,
    const char *body,
    size_t contentLength,
    const char *contentType) const;

  /** Perform a DELETE request with string body */
  [[maybe_unused]] httplib::Result
    deletes(std::string_view path, const std::string &body, const char *contentType) const;

  /** Perform a DELETE request with headers and string body */
  [[maybe_unused]] httplib::Result deletes(std::string_view path,
    const httplib::Headers &headers,
    const std::string &body,
    const char *contentType) const;
};

#endif// HTTP_LIBRARY_CLIENT_H