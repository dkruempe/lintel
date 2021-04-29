#ifndef CPP_BASE_LIBRARY_SERVER_H
#define CPP_BASE_LIBRARY_SERVER_H

#include <boost/beast.hpp>
#include <map>
#include <memory>
#include <thread>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/websocket/Controller.h"
#include "base_library/features/websocket/Session.h"

class Server {
 private:
  std::vector<std::shared_ptr<Controller>> m_controllers;
  boost::asio::io_context m_context;
  boost::asio::ip::tcp::acceptor m_acceptor;
  std::thread m_thread;
  std::map<std::string, std::shared_ptr<Session>> m_sessions;

  void run();

  void onClose(const std::string& id);

  void doAccept();

  void onAccept(boost::beast::error_code errorCode,
                boost::asio::ip::tcp::socket socket);

 public:
  Server(const std::shared_ptr<Configuration>& configuration,
         std::vector<std::shared_ptr<Controller>> controllers);

  ~Server();
  static boost::asio::ip::tcp::endpoint buildEndpoint(
      const std::shared_ptr<Configuration>& sharedPtr);
};

#endif  // CPP_BASE_LIBRARY_SERVER_H
