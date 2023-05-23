#ifndef HTTP_LIBRARY_SERVER_H
#define HTTP_LIBRARY_SERVER_H

#include <httplib.h>

#include <filesystem>
#include <memory>
#include <thread>

#include "Controller.h"
#include "base_library/features/http/configuration/ServerConfiguration.h"

class Server {
private:
    std::vector<std::shared_ptr<Controller>> m_controller;
    std::shared_ptr<httplib::SSLServer> m_sslServer;
    std::shared_ptr<httplib::Server> m_server;
    std::thread m_thread;

    static std::shared_ptr<httplib::Server> initServer(
            const std::vector<std::shared_ptr<Controller>> &controllers,
            const ServerConfiguration &serverConfiguration,
            const std::shared_ptr<httplib::SSLServer> &sslServer);

    static std::shared_ptr<httplib::SSLServer> initSslServer(
            const std::vector<std::shared_ptr<Controller>> &controllers,
            const ServerConfiguration &serverConfiguration);

public:
    Server(const ServerConfiguration &serverConfiguration,
           std::vector<std::shared_ptr<Controller>> controller);

    ~Server();
};

#endif  // HTTP_LIBRARY_SERVER_H
