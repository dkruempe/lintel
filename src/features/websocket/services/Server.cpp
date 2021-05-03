#include "base_library/features/websocket/services/Server.h"

#include <sstream>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/websocket/configuration/WebsocketComponent.h"
#include "base_library/features/websocket/configuration/WebsocketEntry.h"

Server::Server(const std::shared_ptr<Configuration>& configuration,
               std::vector<std::shared_ptr<Controller>> controllers)
    : m_controllers(std::move(controllers)),
      m_context(1),
      m_acceptor(m_context, buildEndpoint(configuration)),
      m_thread([&]() { run(); }) {}

boost::asio::ip::tcp::endpoint Server::buildEndpoint(
    const std::shared_ptr<Configuration>& configuration) {
  std::vector<std::shared_ptr<Entry>> entries =
      configuration->configurationOf<WebsocketComponent>();
  auto found =
      std::find_if(entries.begin(), entries.end(),
                   [](const std::shared_ptr<Entry>& entry) -> bool {
                     std::shared_ptr<WebsocketEntry> websocketEntry =
                         std::static_pointer_cast<WebsocketEntry>(entry);
                     return websocketEntry->getName() == "DEFAULT";
                   });
  std::shared_ptr<WebsocketEntry> websocketEntry =
      std::static_pointer_cast<WebsocketEntry>(*found);
  std::stringstream ss;
  ss << *websocketEntry;
  LOG_INFO("start websocket Server with {}", ss.str());
  return boost::asio::ip::tcp::endpoint(
      boost::asio::ip::address::from_string(websocketEntry->getAddress()),
      websocketEntry->getPort());
}
void Server::run() {
  doAccept();
  m_context.run();
}
void Server::onClose(const std::string& id) { m_sessions.erase(id); }
void Server::doAccept() {
  m_acceptor.async_accept(
      boost::beast::bind_front_handler(&Server::onAccept, this));
}
void Server::onAccept(boost::beast::error_code errorCode,
                      boost::asio::ip::tcp::socket socket) {
  if (errorCode) {
    LOG_ERROR("accept {}", errorCode.message());
    return;
  }

  std::function<void(const std::string&)> temp = [&](const std::string& id) {
    onClose(id);
  };

  auto session =
      std::make_shared<Session>(std::move(socket), temp, m_controllers);
  session->run();
  m_sessions.insert({session->getId(), std::move(session)});
  doAccept();
}
Server::~Server() {
  m_context.stop();
  m_thread.join();
}
