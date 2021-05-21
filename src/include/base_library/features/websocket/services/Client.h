#ifndef CPP_BASE_LIBRARY_CLIENT_H
#define CPP_BASE_LIBRARY_CLIENT_H

#include <boost/beast.hpp>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "base_library/features/base/configuration/Configuration.h"
#include "base_library/features/websocket/configuration/WebsocketEntry.h"
#include "base_library/features/websocket/services/Controller.h"
#include "base_library/features/websocket/services/Session.h"

class Client {
 private:
  std::vector<std::shared_ptr<Controller>> m_controllers;
  std::map<std::string, std::shared_ptr<Controller>> m_methodControllerMap;
  boost::asio::io_context m_context;
  std::shared_ptr<WebsocketEntry> config;
  std::shared_ptr<Session> m_session;
  std::thread m_thread;

  static std::map<std::string, std::shared_ptr<Controller>> build(
      const std::vector<std::shared_ptr<Controller>>& controllers);

  void run();

 public:
  Client(std::shared_ptr<WebsocketEntry> websocketEntry,
         std::vector<std::shared_ptr<Controller>> controllers);
  ~Client();
  void send(const std::shared_ptr<Notification> &notification);
  std::future<std::shared_ptr<Response>> send(
      const std::shared_ptr<Request> &request);
  void send(const std::shared_ptr<Response> &response);
};

#endif  // CPP_BASE_LIBRARY_CLIENT_H
