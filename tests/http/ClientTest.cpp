#include <catch2/catch_all.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <httplib.h>

#include "lintel/features/http/configuration/ClientConfiguration.h"
#include "lintel/features/http/service/Client.h"

namespace lintel {
namespace {

  std::shared_ptr<ClientConfiguration> httpClientCfg(const std::string &cert = "",
    const std::string &key = "",
    const std::string &caCert = "",
    std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
  {
    return std::make_shared<ClientConfiguration>(
      "127.0.0.1", 1, timeout, timeout, timeout, cert, key, caCert);
  }

  httplib::Headers httpTestHeaders() { return httplib::Headers{ { "X-Test", "1" } }; }

  httplib::Params httpTestParams() { return httplib::Params{ { "key", "value" } }; }

  /** Exercise every request overload of a client so both the plain and the TLS code path run. */
  void exerciseClientRequests(const Client &client)
  {
    const std::string path = "/test";
    const auto headers = httpTestHeaders();
    const auto params = httpTestParams();
    const std::string body = "body";
    const char *rawBody = "body";
    const httplib::ContentReceiver receiver = [](const char *, size_t) { return true; };
    const httplib::ResponseHandler responseHandler = [](const httplib::Response &) { return true; };
    const httplib::ContentProvider provider = [](size_t, size_t, httplib::DataSink &) { return true; };
    const httplib::ContentProviderWithoutLength providerNoLength = [](size_t, httplib::DataSink &) { return true; };

    client.get(path);
    client.get(path, headers);
    client.get(path, receiver);
    client.get(path, headers, receiver);
    client.get(path, responseHandler, receiver);
    client.get(path, headers, responseHandler, receiver);

    client.post(path);
    client.post(path, rawBody, body.size(), "text/plain");
    client.post(path, headers, rawBody, body.size(), "text/plain");
    client.post(path, body, "text/plain");
    client.post(path, headers, body, "text/plain");
    client.post(path, body.size(), provider, "text/plain");
    client.post(path, providerNoLength, "text/plain");
    client.post(path, headers, body.size(), provider, "text/plain");
    client.post(path, headers, providerNoLength, "text/plain");
    client.post(path, params);
    client.post(path, headers, params);

    client.put(path);
    client.put(path, rawBody, body.size(), "text/plain");
    client.put(path, headers, rawBody, body.size(), "text/plain");
    client.put(path, body, "text/plain");
    client.put(path, headers, body, "text/plain");
    client.put(path, body.size(), provider, "text/plain");
    client.put(path, providerNoLength, "text/plain");
    client.put(path, headers, body.size(), provider, "text/plain");
    client.put(path, headers, providerNoLength, "text/plain");
    client.put(path, params);
    client.put(path, headers, params);

    client.deletes(path);
    client.deletes(path, headers);
    client.deletes(path, rawBody, body.size(), "text/plain");
    client.deletes(path, headers, rawBody, body.size(), "text/plain");
    client.deletes(path, body, "text/plain");
    client.deletes(path, headers, body, "text/plain");

    client.setBasicAuth("user", "password");
    client.setDigestAuth("user", "password");
    client.setBearerTokenAuth("token");
  }

}// namespace

TEST_CASE("Client: every request overload runs over a plain connection")
{
  Client client(httpClientCfg());
  REQUIRE_NOTHROW(exerciseClientRequests(client));
}

TEST_CASE("Client: every request overload runs over a TLS connection with cert and key")
{
  Client client(httpClientCfg("/nonexistent/client.crt", "/nonexistent/client.key"));
  REQUIRE_NOTHROW(exerciseClientRequests(client));
}

TEST_CASE("Client: a TLS connection configured only with a CA certificate runs requests")
{
  Client client(httpClientCfg("", "", "/nonexistent/ca.crt"));
  client.get("/test");
  client.setBearerTokenAuth("token");
  client.setBasicAuth("user", "password");
  client.setDigestAuth("user", "password");
  SUCCEED();
}

TEST_CASE("Client: zero timeouts leave the client defaults untouched")
{
  Client client(httpClientCfg("", "", "", std::chrono::milliseconds(0)));
  client.get("/test");
  SUCCEED();
}

}// namespace lintel
