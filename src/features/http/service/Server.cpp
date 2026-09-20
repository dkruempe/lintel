#include "base_library/features/http/service/Server.h"

#include <httplib.h>

#include <stdexcept>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/http/service/ContentType.h"
#include "base_library/features/http/service/Controller.h"
#include "base_library/features/http/service/HttpStatusCodes.h"

namespace {
/** Body of the liveness (health) response */
constexpr const char *kHealthBody = "{\"status\":\"ok\"}";
/** Body of the readiness response */
constexpr const char *kReadyBody = "{\"status\":\"ready\"}";

/** Register unauthenticated liveness/readiness endpoints on the server */
void registerHealthEndpoints(std::shared_ptr<httplib::Server> server)
{
  server->Get("/health", [](const httplib::Request &, httplib::Response &response) {
    response.status = HttpStatusCodes::OK;
    response.set_content(kHealthBody, ContentType(ContentType::ApplicationJson).getName());
  });
  server->Get("/ready", [](const httplib::Request &, httplib::Response &response) {
    response.status = HttpStatusCodes::OK;
    response.set_content(kReadyBody, ContentType(ContentType::ApplicationJson).getName());
  });
}

/** Initialize a plain HTTP server */
std::shared_ptr<httplib::Server> initServer(const std::vector<std::shared_ptr<Controller>> &controllers,
  const ServerConfiguration &serverConfiguration,
  const std::shared_ptr<httplib::SSLServer> &sslServer)
{
  for (const auto &controller : controllers) { controller->setTrustedProxies(serverConfiguration.getTrustedProxies()); }
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
  std::shared_ptr<httplib::SSLServer> sslServer = std::make_shared<httplib::SSLServer>(
    serverConfiguration.getCertFile().c_str(), serverConfiguration.getKeyFile().c_str());
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
  std::thread thread;

  Impl(const ServerConfiguration &serverConfiguration, std::vector<std::shared_ptr<Controller>> controllers)
    : controller(std::move(controllers)), sslServer(initSslServer(controller, serverConfiguration)),
      server(initServer(controller, serverConfiguration, sslServer))
  {
    const std::string host = serverConfiguration.getHost();
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