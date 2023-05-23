#include "base_library/features/http/service/Server.h"

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
    if (sslServer != nullptr) {
        return std::static_pointer_cast<httplib::Server>(sslServer);
    }
    std::shared_ptr<httplib::Server> server = std::make_shared<httplib::Server>();
    for (const auto &iter: controllers) {
        iter->registerMethods(server);
    }
    server->set_write_timeout(serverConfiguration.getWriteTimeOut());
    server->set_read_timeout(serverConfiguration.getReadTimeOut());
    server->set_idle_interval(serverConfiguration.getIdleInterval());
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
    sslServer->set_write_timeout(serverConfiguration.getWriteTimeOut());
    sslServer->set_read_timeout(serverConfiguration.getReadTimeOut());
    sslServer->set_idle_interval(serverConfiguration.getIdleInterval());
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
