#include "base_library/features/websocket/services/Session.h"

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/UUID.h"
#include "base_library/features/websocket/messages/ErrorMessage.h"
#include "base_library/features/websocket/services/Controller.h"

/**
 * constructor for server session
 * @param socket
 * @param onClose
 * @param controllers
 */
Session::Session(boost::asio::ip::tcp::socket &&socket,
                 std::function<void(const std::string &)> onClose,
                 std::map<std::string, std::shared_ptr<Controller>> controllers)
    : m_id(UUID::generate()),
      m_websocket(std::move(socket)),
      m_onClose(std::move(onClose)),
      m_controllers(std::move(controllers)),
      messageFactory(),
      processingRequests() {
  m_websocket.set_option(
      boost::beast::websocket::stream_base::timeout::suggested(
          boost::beast::role_type::server));
  m_websocket.set_option(boost::beast::websocket::stream_base::decorator(
      [](boost::beast::websocket::response_type &res) {
        res.set(boost::beast::http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + "ws-simple-server");
      }));
  for (const auto &[name, controller] : m_controllers) {
    for (const auto &[methodName, func] : controller->getRequests()) {
      messageFactory.registerRequest(methodName, func);
    }
    for (const auto &[methodName, func] : controller->getResponses()) {
      messageFactory.registerResponse(methodName, func);
    }
  }
}

/**
 * constructor for client session
 * @param context
 * @param onClose
 * @param controllers
 * @param websocketEntry
 */
Session::Session(boost::asio::io_context &context,
                 std::function<void(const std::string &)> onClose,
                 std::map<std::string, std::shared_ptr<Controller>> controllers,
                 std::shared_ptr<WebsocketEntry> websocketEntry)
    : m_id(UUID::generate()),
      m_websocketEntry(std::move(websocketEntry)),
      m_websocket(boost::asio::make_strand(context)),
      m_onClose(std::move(onClose)),
      m_controllers(std::move(controllers)),
      messageFactory(),
      processingRequests(),
      m_resolver(std::make_unique<boost::asio::ip::tcp::resolver>(
          boost::asio::make_strand(context))),
      m_host(m_websocketEntry->getAddress()),
      m_port(std::to_string(m_websocketEntry->getPort())) {
  for (const auto &[name, controller] : m_controllers) {
    for (const auto &[methodName, func] : controller->getRequests()) {
      messageFactory.registerRequest(methodName, func);
    }
    for (const auto &[methodName, func] : controller->getResponses()) {
      messageFactory.registerResponse(methodName, func);
    }
  }
}
void Session::onResolve(boost::beast::error_code ec,
                        boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) {
    return fail(ec, "resolve");
  }

  // Set the timeout for the operation
  boost::beast::get_lowest_layer(m_websocket)
      .expires_after(std::chrono::seconds(30));
  // Make the connection on the IP address we get from a lookup
  boost::beast::get_lowest_layer(m_websocket)
      .async_connect(results, boost::beast::bind_front_handler(
                                  &Session::onConnect, shared_from_this()));
}
void Session::run() {
  if (m_websocketEntry == nullptr) {
    m_websocket.async_accept(boost::beast::bind_front_handler(
        &Session::onAccept, shared_from_this()));
  } else {
    // Look up the domain name
    m_resolver->async_resolve(m_host.c_str(), m_port.c_str(),
                              boost::beast::bind_front_handler(
                                  &Session::onResolve, shared_from_this()));
  }
}
void Session::doRead() {
  m_websocket.async_read(
      m_readBuffer,
      boost::beast::bind_front_handler(&Session::onRead, shared_from_this()));
}
void Session::onAccept(boost::beast::error_code errorCode) {
  if (errorCode) {
    fail(errorCode, "accept");
    return;
  }
  doRead();
}
void Session::onHandshake(boost::beast::error_code ec) {
  if (ec) {
    fail(ec, "handshake");
    return;
  }
  // read messages
  doRead();
}
void Session::onConnect(
    boost::beast::error_code ec,
    boost::asio::ip::tcp::resolver::results_type::endpoint_type ep) {
  if (ec) {
    fail(ec, "connect");
    return;
  }

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  boost::beast::get_lowest_layer(m_websocket).expires_never();

  // Set suggested timeout settings for the websocket
  m_websocket.set_option(
      boost::beast::websocket::stream_base::timeout::suggested(
          boost::beast::role_type::client));

  // Set a decorator to change the User-Agent of the handshake
  m_websocket.set_option(boost::beast::websocket::stream_base::decorator(
      [](boost::beast::websocket::request_type &req) {
        req.set(boost::beast::http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-async");
      }));

  // Update the host_ string. This will provide the value of the
  // Host HTTP header during the WebSocket handshake.
  // See https://tools.ietf.org/html/rfc7230#section-5.4
  std::string host =
      m_websocketEntry->getAddress() + ":" + std::to_string(ep.port());

  // Perform the websocket handshake
  m_websocket.async_handshake(host, "/",
                              boost::beast::bind_front_handler(
                                  &Session::onHandshake, shared_from_this()));
}
void Session::onRead(boost::beast::error_code errorCode, size_t length) {
  if (errorCode == boost::beast::websocket::error::closed) {
    close();
    return;
  }
  if (errorCode) {
    fail(errorCode, "read");
    close();
    return;
  }
  if (m_websocket.got_text()) {
    auto data = static_cast<const char *>(m_readBuffer.cdata().data());
    std::string textBuffer;
    textBuffer.assign(data, data + m_readBuffer.size());
    auto packet = std::make_shared<Packet>(textBuffer);
    onReceive(packet);
  } else if (m_websocket.got_binary()) {
    const uint8_t *temp =
        static_cast<const uint8_t *>(m_readBuffer.cdata().data());
    std::vector<uint8_t> binaryBuffer;
    binaryBuffer.reserve(m_readBuffer.size());
    for (std::size_t i = 0; i < m_readBuffer.size(); i++) {
      binaryBuffer.push_back(temp[i]);
    }
    auto packet = std::make_shared<Packet>(binaryBuffer);
    onReceive(packet);
  }
  m_readBuffer.clear();
  doRead();
}
void Session::onReceive(const std::shared_ptr<Packet> &packet) {
  if (packet->isText()) {
    try {
      MessageContainer container =
          messageFactory.generate(packet->getTextBuffer(), processingRequests);
      LOG_INFO("created messages {}", container.getMessages().size());
      for (const auto &item : container.getMessages()) {
        LOG_INFO("received message {}", item->serialize());
        switch (item->getType()) {
          case Message::NOTIFICATION: {
            std::shared_ptr<Notification> notification =
                std::static_pointer_cast<Notification>(item);
            try {
              m_controllers.at(notification->getMethod())
                  ->onReceive(notification);
            } catch (std::out_of_range &e) {
              LOG_ERROR("method {} not found", notification->getMethod());
              send(std::make_shared<ErrorMessage>(ErrorCode::METHOD_NOT_FOUND,
                                                  "method not found", ""));
            }
            break;
          }
          case Message::REQUEST: {
            std::shared_ptr<Request> request =
                std::static_pointer_cast<Request>(item);
            try {
              std::shared_ptr<Response> response =
                  m_controllers.at(request->getMethod())->onReceive(request);
              if (response == nullptr) {
                LOG_ERROR("response is nullpointer ERROR");
              } else {
                send(response);
              }
            } catch (std::out_of_range &e) {
              LOG_ERROR("method {} not found", request->getMethod());
              send(std::make_shared<ErrorMessage>(ErrorCode::METHOD_NOT_FOUND,
                                                  "method not found",
                                                  request->getId()));
            }
            break;
          }
          case Message::RESPONSE: {
            std::shared_ptr<Response> response =
                std::static_pointer_cast<Response>(item);
            try {
              m_pendingRequests.at(response->getId()).set_value(response);
            } catch (std::out_of_range &e) {
              LOG_ERROR("id not found {} in pending requests",
                        response->getId());
              send(std::make_shared<ErrorMessage>(
                  ErrorCode::METHOD_NOT_FOUND,
                  "no matching waiting response for request found",
                  response->getId()));
            }
            break;
          }
        }
      }
      for (const auto &item : container.getErrors()) {
        std::shared_ptr<Response> temp =
            std::static_pointer_cast<Response>(item);
        send(temp);
      }
    } catch (std::exception &e) {
      std::shared_ptr<Response> response = std::make_shared<ErrorMessage>(
          ErrorCode::PARSE_ERROR, "parse error", "");
      send(response);
      LOG_ERROR("{} {}", e.what(), packet->getTextBuffer());
    }
  } else {
    std::string temp;
    for (auto &iter : packet->getBinaryBuffer()) {
      temp += std::to_string(iter);
      temp += ",";
    }
    LOG_INFO("Received: {}", temp);
  }
}
void Session::onWrite(boost::beast::error_code errorCode, std::size_t length) {
  if (errorCode) {
    fail(errorCode, "write");
    return;
  }
  m_writeQueue.pop_front();
  if (m_writeQueue.empty()) {
    return;
  }
  doWrite();
}
void Session::doWrite() {
  if (m_writeQueue.empty()) {
    return;
  }
  auto &packet = m_writeQueue.front();
  m_websocket.text(packet->isText());
  auto handler =
      boost::beast::bind_front_handler(&Session::onWrite, shared_from_this());
  if (packet->isText()) {
    m_websocket.async_write(boost::asio::buffer(packet->getTextBuffer()),
                            std::move(handler));
  } else {
    m_websocket.async_write(boost::asio::buffer(packet->getBinaryBuffer()),
                            std::move(handler));
  }
}
void Session::fail(boost::beast::error_code errorCode,
                   const std::string &message) const {
  LOG_ERROR("Session {}-{}: {} {}", m_id, message, errorCode.message(),
            errorCode.value());
}

void Session::close() { m_onClose(m_id); }
void Session::send(const std::shared_ptr<Packet> &packet) {
  m_writeQueue.emplace_back(packet);
  if (m_writeQueue.size() > 1) {
    return;
  }
  doWrite();
}
const std::string &Session::getId() { return m_id; }
void Session::send(const std::shared_ptr<Notification> &notification) {
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(notification->serialize());
  send(packet);
}
std::future<std::shared_ptr<Response>> Session::send(
    const std::shared_ptr<Request> &request) {
  std::promise<std::shared_ptr<Response>> promise;
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(request->serialize());
  send(packet);
  std::string id = request->getId();
  std::pair<std::string, std::promise<std::shared_ptr<Response>>> pair{
      std::move(id), std::move(promise)};
  m_pendingRequests.insert(std::move(pair));
  processingRequests.waitingFor(request->getMethod(), request->getId());
  return m_pendingRequests.at(request->getId()).get_future();
}
void Session::send(const std::shared_ptr<Response> &response) {
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(response->serialize());
  send(packet);
}
