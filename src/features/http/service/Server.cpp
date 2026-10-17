#include "lintel/features/http/service/Server.h"

#include <httplib.h>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/services/ReadinessRegistry.h"
#include "lintel/features/http/service/ContentType.h"
#include "lintel/features/http/service/Controller.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/TlsReloadRegistry.h"

namespace lintel {
namespace {
  /** Body of the liveness (health) response */
  constexpr const char *kHealthBody = "{\"status\":\"ok\"}";
  /** Body of the readiness response while no dependency check is registered */
  constexpr const char *kReadyBody = "{\"status\":\"ready\"}";
  /** Name the HTTP server registers its TLS reload handler under */
  constexpr const char *kTlsReloadName = "tls";

  /** Read a whole file in binary mode. */
  bool readFile(const std::string &fileName, std::string &content)
  {
    std::ifstream file(fileName, std::ios::binary);
    if (!file) { return false; }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    return true;
  }

  /** Reload the certificate/key pair of a running TLS server.
   *
   * httplib loads the pair into the OpenSSL context once, so a renewed file on
   * disk stays invisible until the process restarts. update_certs_pem() swaps
   * it under the context mutex the handshake path also takes, so a handshake
   * starting at the same time sees either the old or the new pair.
   *
   * @param sslServer    the running server, nullptr for a clear-text server
   * @param certFile     certificate file to read
   * @param keyFile      private key file to read
   * @param lastGoodCert PEM of the pair currently in use, updated on success
   * @param lastGoodKey  PEM of the pair currently in use, updated on success
   * @return an error description, empty on success */
  std::string reloadTls(const std::shared_ptr<httplib::SSLServer> &sslServer,
    const std::string &certFile,
    const std::string &keyFile,
    std::string &lastGoodCert,
    std::string &lastGoodKey)
  {
    if (sslServer == nullptr) { return "the HTTP server is not running with TLS"; }
    std::string cert;
    if (!readFile(certFile, cert)) { return "cannot read the certificate file '" + certFile + "'"; }
    std::string key;
    if (!readFile(keyFile, key)) { return "cannot read the private key file '" + keyFile + "'"; }
    if (sslServer->update_certs_pem(cert.c_str(), key.c_str())) {
      lastGoodCert = std::move(cert);
      lastGoodKey = std::move(key);
      LOG_INFO("reloaded the TLS certificate from '{}'", certFile);
      return {};
    }
    // update_certs_pem() rejects the pair, but it sets the certificate before it
    // validates the key: a mismatched renewal would leave the context with the
    // new certificate and the old key, and every following handshake would
    // fail. Re-applying the pair that is known to work keeps the server on the
    // old certificate until the operator fixes the renewal files.
    if (!lastGoodCert.empty() && !lastGoodKey.empty()) {
      sslServer->update_certs_pem(lastGoodCert.c_str(), lastGoodKey.c_str());
    }
    return "the certificate/key pair in '" + certFile + "' and '" + keyFile + "' was rejected by OpenSSL";
  }

  /**
   * @return the readiness body with the result of every registered check:
   *         {"status":"ready|not_ready","checks":{"<name>":"ok|failed: <reason>"}}
   *
   * rapidjson writes the JSON so that failure reasons coming from exception
   * messages (quotes, backslashes, control characters) cannot break the body.
   */
  std::string readinessBody(const std::vector<ReadinessRegistry::CheckResult> &results, const bool ready)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("status");
    writer.String(ready ? "ready" : "not_ready");
    writer.Key("checks");
    writer.StartObject();
    for (const auto &result : results) {
      writer.Key(result.m_name.c_str(), static_cast<rapidjson::SizeType>(result.m_name.size()));
      if (result.m_ready) {
        writer.String("ok");
      } else {
        const std::string detail = result.m_detail.empty() ? "failed" : "failed: " + result.m_detail;
        writer.String(detail.c_str(), static_cast<rapidjson::SizeType>(detail.size()));
      }
    }
    writer.EndObject();
    writer.EndObject();
    return std::string(buffer.GetString(), buffer.GetSize());
  }

  /** Register unauthenticated liveness/readiness endpoints on the server */
  void registerHealthEndpoints(const std::shared_ptr<httplib::Server> &server)
  {
    server->Get("/health", [](const httplib::Request &, httplib::Response &response) {
      response.status = HttpStatusCodes::OK;
      response.set_content(kHealthBody, ContentType(ContentType::ApplicationJson).getName());
    });
    server->Get("/ready", [](const httplib::Request &, httplib::Response &response) {
      // liveness stays dependency free, readiness runs the registered checks:
      // an unready dependency must take this instance out of the load balancer
      const std::vector<ReadinessRegistry::CheckResult> results = ReadinessRegistry::instance().run();
      if (results.empty()) {
        response.status = HttpStatusCodes::OK;
        response.set_content(kReadyBody, ContentType(ContentType::ApplicationJson).getName());
        return;
      }
      bool ready = true;
      for (const auto &result : results) { ready = ready && result.m_ready; }
      response.status = ready ? HttpStatusCodes::OK : HttpStatusCodes::ServiceUnavailable;
      response.set_content(readinessBody(results, ready), ContentType(ContentType::ApplicationJson).getName());
    });
  }

  /** Initialize a plain HTTP server */
  std::shared_ptr<httplib::Server> initServer(const std::vector<std::shared_ptr<Controller>> &controllers,
    const ServerConfiguration &serverConfiguration,
    const std::shared_ptr<httplib::SSLServer> &sslServer)
  {
    for (const auto &controller : controllers) {
      controller->setTrustedProxies(serverConfiguration.getTrustedProxies());
    }
    if (sslServer != nullptr) { return std::static_pointer_cast<httplib::Server>(sslServer); }
    if (serverConfiguration.isTlsRequired()) {
      throw std::runtime_error(
        "require_tls is enabled but no TLS certificate/key is "
        "configured; refusing to start the HTTP server in clear text");
    }
    LOG_WARN(
      "HTTP server is running without TLS; credentials are transmitted "
      "in clear text. Configure cert/key files for production use.");
    std::shared_ptr<httplib::Server> server = std::make_shared<httplib::Server>();
    // httplib leaves Nagle on (CPPHTTPLIB_TCP_NODELAY defaults to false), which
    // adds a delayed-ack stall of up to 40 ms to small responses. An API server
    // answers with small JSON bodies, so latency wins over the packing that
    // Nagle does for bulk transfers - and Nagle never delays full segments.
    server->set_tcp_nodelay(true);
    server->set_error_logger([](const httplib::Error &error, const httplib::Request *) {
      LOG_ERROR("httplib server error: {}", httplib::to_string(error));
    });
    for (const auto &iter : controllers) { iter->registerMethods(server); }
    registerHealthEndpoints(server);
    if (serverConfiguration.getWriteTimeOut().count() != 0) {
      server->set_write_timeout(serverConfiguration.getWriteTimeOut());
    }
    if (serverConfiguration.getReadTimeOut().count() != 0) {
      server->set_read_timeout(serverConfiguration.getReadTimeOut());
    }
    if (serverConfiguration.getIdleInterval().count() != 0) {
      server->set_idle_interval(serverConfiguration.getIdleInterval());
    }
    server->set_payload_max_length(8ULL * 1024ULL * 1024ULL);
    return server;
  }

  /** Initialize an HTTPS server with SSL */
  std::shared_ptr<httplib::SSLServer> initSslServer(const std::vector<std::shared_ptr<Controller>> &controllers,
    const ServerConfiguration &serverConfiguration)
  {
    if (serverConfiguration.getCertFile().empty() || serverConfiguration.getKeyFile().empty()) { return nullptr; }
    // httplib takes narrow paths; std::filesystem::path::c_str() is wchar_t on Windows.
    const std::string certFile = serverConfiguration.getCertFile().string();
    const std::string keyFile = serverConfiguration.getKeyFile().string();
    std::shared_ptr<httplib::SSLServer> sslServer =
      std::make_shared<httplib::SSLServer>(certFile.c_str(), keyFile.c_str());
    // see initServer: Nagle off, or every small TLS record can wait for the
    // peer's delayed acknowledgement
    sslServer->set_tcp_nodelay(true);
    if (serverConfiguration.getWriteTimeOut().count() != 0) {
      sslServer->set_write_timeout(serverConfiguration.getWriteTimeOut());
    }
    if (serverConfiguration.getReadTimeOut().count() != 0) {
      sslServer->set_read_timeout(serverConfiguration.getReadTimeOut());
    }
    if (serverConfiguration.getIdleInterval().count() != 0) {
      sslServer->set_idle_interval(serverConfiguration.getIdleInterval());
    }
    sslServer->set_payload_max_length(8ULL * 1024ULL * 1024ULL);
    sslServer->set_error_logger([](const httplib::Error &error, const httplib::Request *) {
      LOG_ERROR("httplib ssl server error: {}", httplib::to_string(error));
    });
    std::shared_ptr<httplib::Server> server = std::static_pointer_cast<httplib::Server>(sslServer);
    for (const auto &iter : controllers) { iter->registerMethods(server); }
    registerHealthEndpoints(server);
    return sslServer;
  }
}// namespace

struct Server::Impl
{
  std::vector<std::shared_ptr<Controller>> controller;
  std::shared_ptr<httplib::SSLServer> sslServer;
  std::shared_ptr<httplib::Server> server;
  std::string certFile;
  std::string keyFile;
  std::string lastGoodCert;
  std::string lastGoodKey;
  TlsReloadRegistry::ReloadId tlsReloadId = 0;
  std::thread thread;

  Impl(const ServerConfiguration &serverConfiguration, std::vector<std::shared_ptr<Controller>> controllers)
    : controller(std::move(controllers)), sslServer(initSslServer(controller, serverConfiguration)),
      server(initServer(controller, serverConfiguration, sslServer)),
      certFile(serverConfiguration.getCertFile().string()), keyFile(serverConfiguration.getKeyFile().string())
  {
    if (sslServer != nullptr) {
      // A certificate that OpenSSL cannot load leaves the context empty; without
      // this check the failure surfaces later as "failed to bind", which hides
      // the real cause (a bad or unreadable certificate file).
      if (!sslServer->is_valid()) {
        throw std::runtime_error(
          "HTTP server: cannot load the TLS certificate '" + certFile + "' and key '" + keyFile + "'");
      }
      // Keep the pair that is known to work so a failed reload can roll back to
      // it (see reloadTls) instead of leaving a broken context serving traffic.
      readFile(certFile, lastGoodCert);
      readFile(keyFile, lastGoodKey);
      tlsReloadId = TlsReloadRegistry::instance().registerHandler(
        kTlsReloadName, [this]() { return reloadTls(sslServer, certFile, keyFile, lastGoodCert, lastGoodKey); });
    }
    const std::string &host = serverConfiguration.getHost();
    const int port = serverConfiguration.getPort();
    const bool bound = sslServer != nullptr ? sslServer->bind_to_port(host, port) : server->bind_to_port(host, port);
    if (!bound) { throw std::runtime_error("HTTP server: failed to bind to " + host + ":" + std::to_string(port)); }
    thread = std::thread([this]() {
      if (sslServer != nullptr) {
        sslServer->listen_after_bind();
      } else {
        server->listen_after_bind();
      }
    });
  }

  ~Impl()
  {
    if (sslServer != nullptr) {
      // Drop the reload handler before the server it points at goes away: the
      // registry outlives the server, and a stale handler would let
      // POST /admin/reload-tls report success for a stopped server. Removing it
      // by id keeps a server that took over the name (see ServerProvider) intact.
      TlsReloadRegistry::instance().unregisterHandler(tlsReloadId);
      sslServer->stop();
    } else {
      server->stop();
    }
    thread.join();
  }
};

Server::Server(const ServerConfiguration &serverConfiguration, std::vector<std::shared_ptr<Controller>> controller)
  : m_impl(std::make_unique<Impl>(serverConfiguration, std::move(controller)))
{}

Server::~Server() = default;
}// namespace lintel