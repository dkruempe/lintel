#include "base_library/features/websocket/services/Session.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/core/utils/UUID.h"
#include "base_library/features/websocket/messages/ErrorMessage.h"
#include "base_library/features/websocket/services/Controller.h"

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
}
void Session::run() {
  m_websocket.async_accept(
      boost::beast::bind_front_handler(&Session::onAccept, shared_from_this()));
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
            break;
          }
        }
      }
      for (const auto &item : container.getErrors()) {
        std::shared_ptr<Response> temp =
            std::static_pointer_cast<Response>(item);
        send(temp);
      }
      /*const std::vector<Request> &requests =
          Request::fromJson(packet->textBuffer);
      std::vector<Response> responses;
      for (const auto &request : requests) {
        auto found = controllers.find(request.getMethod());
        if (found == controllers.end()) {
          continue;
        }
        responses.push_back(found->second->onRequest(request));
      }*/
      // send();
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
void Session::send(const std::shared_ptr<Request> &request) {
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(request->serialize());
  send(packet);
}
void Session::send(const std::shared_ptr<Response> &response) {
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(response->serialize());
  send(packet);
}
