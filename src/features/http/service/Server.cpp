#include "base_library/features/http/service/Server.h"

#include <stdexcept>

#include "base_library/core/services/LoggerService.h"

Server::~Server() {
    if (m_sslServer != nullptr) {
        m_sslServer->stop();
    } else {
        m_server->stop();
    }
    m_thread.join();
}

std::shared_ptr<httplib::Server> Server::initServer(
        const std::vector<std::shared_ptr<Controller>> &controllers,
        const ServerConfiguration &serverConfiguration,
        const std::shared_ptr<httplib::SSLServer> &sslServer) {
    for (const auto &controller: controllers) {
        controller->setTrustedProxies(serverConfiguration.getTrustedProxies());
    }
    if (sslServer != nullptr) {
        return std::static_pointer_cast<httplib::Server>(sslServer);
    }
    if (serverConfiguration.isTlsRequired()) {
        throw std::runtime_error(
                "require_tls is enabled but no TLS certificate/key is "
                "configured; refusing to start the HTTP server in clear text");
    }
    LOG_WARN("HTTP server is running without TLS; credentials are transmitted "
             "in clear text. Configure cert/key files for production use.");
    std::shared_ptr<httplib::Server> server = std::make_shared<httplib::Server>();
    server->set_error_logger([](const httplib::Error &error,
                                const httplib::Request *) {
        LOG_ERROR("httplib server error: {}", httplib::to_string(error));
    });
    for (const auto &iter: controllers) {
        iter->registerMethods(server);
    }
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

std::shared_ptr<httplib::SSLServer> Server::initSslServer(
        const std::vector<std::shared_ptr<Controller>> &controllers,
        const ServerConfiguration &serverConfiguration) {
    if (serverConfiguration.getCertFile().empty() ||
        serverConfiguration.getKeyFile().empty()) {
        return nullptr;
    }
    std::shared_ptr<httplib::SSLServer> sslServer =
            std::make_shared<httplib::SSLServer>(
                    serverConfiguration.getCertFile().c_str(),
                    serverConfiguration.getKeyFile().c_str());
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
    sslServer->set_error_logger([](const httplib::Error &error,
                                   const httplib::Request *) {
        LOG_ERROR("httplib ssl server error: {}", httplib::to_string(error));
    });
    std::shared_ptr<httplib::Server> server =
            std::static_pointer_cast<httplib::Server>(sslServer);
    for (const auto &iter: controllers) {
        iter->registerMethods(server);
    }
    return sslServer;
}

Server::Server(const ServerConfiguration &serverConfiguration,
               std::vector<std::shared_ptr<Controller>> controller)
        : m_controller(std::move(controller)),
          m_sslServer(initSslServer(m_controller, serverConfiguration)),
          m_server(initServer(m_controller, serverConfiguration, m_sslServer)),
          m_thread([serverConfiguration, this]() {
              if (m_sslServer != nullptr) {
                  m_sslServer->listen(serverConfiguration.getHost().c_str(),
                                      serverConfiguration.getPort());
              } else {
                  m_server->listen(serverConfiguration.getHost().c_str(),
                                   serverConfiguration.getPort());
              }
          }) {}
