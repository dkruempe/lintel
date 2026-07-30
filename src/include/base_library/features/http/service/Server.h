#ifndef HTTP_LIBRARY_SERVER_H
#define HTTP_LIBRARY_SERVER_H

#include <httplib.h>

#include <filesystem>
#include <memory>
#include <thread>

#include "Controller.h"
#include "base_library/features/http/configuration/ServerConfiguration.h"

/** HTTP server that manages controller registration and runs in a background thread */
class Server {
private:
    std::vector<std::shared_ptr<Controller>> m_controller;
    std::shared_ptr<httplib::SSLServer> m_sslServer;
    std::shared_ptr<httplib::Server> m_server;
    std::thread m_thread;

    /** Initialize a plain HTTP server */
    static std::shared_ptr<httplib::Server> initServer(
            const std::vector<std::shared_ptr<Controller>> &controllers,
            const ServerConfiguration &serverConfiguration,
            const std::shared_ptr<httplib::SSLServer> &sslServer);

    /** Initialize an HTTPS server with SSL */
    static std::shared_ptr<httplib::SSLServer> initSslServer(
            const std::vector<std::shared_ptr<Controller>> &controllers,
            const ServerConfiguration &serverConfiguration);

public:
    /** @param serverConfiguration server settings
     *  @param controller list of controllers to register */
    Server(const ServerConfiguration &serverConfiguration,
           std::vector<std::shared_ptr<Controller>> controller);

    ~Server();
};

#endif  // HTTP_LIBRARY_SERVER_H
