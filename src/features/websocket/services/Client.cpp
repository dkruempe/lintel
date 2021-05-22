#include "base_library/features/websocket/services/Client.h"

Client::Client(std::shared_ptr<WebsocketEntry> websocketEntry,
               std::vector<std::shared_ptr<Controller>> controllers)
    : m_controllers(std::move(controllers)),
      m_methodControllerMap(build(m_controllers)),
      config(std::move(websocketEntry)),
      m_session(std::make_shared<Session>(
          m_context, [](const std::string&) {}, m_methodControllerMap, config)),
      m_thread([&]() { run(); }) {}
void Client::send(const std::shared_ptr<Notification>& notification) {
  m_session->send(notification);
}
std::future<std::shared_ptr<Response>> Client::send(
    const std::shared_ptr<Request>& request) {
  return m_session->send(request);
}
void Client::send(const std::shared_ptr<Response>& response) {
  m_session->send(response);
}

std::map<std::string, std::shared_ptr<Controller>> Client::build(
    const std::vector<std::shared_ptr<Controller>>& controllers) {
  std::map<std::string, std::shared_ptr<Controller>> map;
  for (auto& controller : controllers) {
    for (const auto& method : controller->getMethods()) {
      map.insert({method, controller});
    }
  }
  return map;
}
void Client::run() {
  m_session->run();
  m_context.run();
}
Client::~Client() {
  m_context.stop();
  m_thread.join();
}
bool Client::isConnected() { return m_session->isConnected(); }
