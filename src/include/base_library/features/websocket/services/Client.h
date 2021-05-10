#ifndef CPP_BASE_LIBRARY_CLIENT_H
#define CPP_BASE_LIBRARY_CLIENT_H

#include <boost/beast.hpp>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/websocket/services/Controller.h"
#include "base_library/features/websocket/services/Session.h"

class Client {
 private:
  std::vector<std::shared_ptr<Controller>> m_controllers;
  std::map<std::string, std::shared_ptr<Controller>> m_methodControllerMap;
  boost::asio::io_context m_context;
  boost::asio::ip::tcp::resolver m_resolver;
  std::thread m_thread;

  static std::map<std::string, std::shared_ptr<Controller>> build(
      const std::vector<std::shared_ptr<Controller>>& controllers);

  void run();

 public:
  Client(const std::shared_ptr<Configuration>& configuration,
         std::vector<std::shared_ptr<Controller>> controllers);
  ~Client();
};

#endif  // CPP_BASE_LIBRARY_CLIENT_H
