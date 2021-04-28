#include "base_library/features/websocket/Session.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/websocket/Controller.h"
#include "base_library/features/websocket/MessageFactory.h"

Session::Session(boost::asio::ip::tcp::socket &&socket,
                 std::function<void(const std::string &)> onClose,
                 std::vector<std::shared_ptr<Controller>> controllers)
    : id(boost::uuids::to_string(boost::uuids::random_generator()())),
      websocket(std::move(socket)),
      onClose(std::move(onClose)),
      controllers(std::move(controllers)) {
  websocket.set_option(boost::beast::websocket::stream_base::timeout::suggested(
      boost::beast::role_type::server));
  websocket.set_option(boost::beast::websocket::stream_base::decorator(
      [](boost::beast::websocket::response_type &res) {
        res.set(boost::beast::http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + "ws-simple-server");
      }));
}
void Session::run() {
  websocket.async_accept(
      boost::beast::bind_front_handler(&Session::onAccept, shared_from_this()));
}
void Session::doRead() {
  websocket.async_read(readBuffer, boost::beast::bind_front_handler(
                                       &Session::onRead, shared_from_this()));
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
  if (websocket.got_text()) {
    auto data = static_cast<const char *>(readBuffer.cdata().data());
    std::string textBuffer;
    textBuffer.assign(data, data + readBuffer.size());
    auto packet = std::make_shared<Packet>(textBuffer);
    onReceive(packet);
  } else if (websocket.got_binary()) {
    const uint8_t *temp =
        static_cast<const uint8_t *>(readBuffer.cdata().data());
    std::vector<uint8_t> binaryBuffer;
    binaryBuffer.reserve(readBuffer.size());
    for (std::size_t i = 0; i < readBuffer.size(); i++) {
      binaryBuffer.push_back(temp[i]);
    }
    auto packet = std::make_shared<Packet>(binaryBuffer);
    onReceive(packet);
  }
  readBuffer.clear();
  doRead();
}
void Session::onReceive(const std::shared_ptr<Packet> &packet) {
  if (packet->isText()) {
    try {
      std::vector<std::shared_ptr<Message>> messages =
          MessageFactory::generate(packet->getTextBuffer());
      LOG_INFO("created messages {}", messages.size());
      for (const auto &item : messages) {
        std::stringstream ss;
        ss << *item;
        LOG_INFO("received message {}", ss.str());
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
  writeQueue.pop_front();
  if (writeQueue.empty()) {
    return;
  }
  doWrite();
}
void Session::doWrite() {
  if (writeQueue.empty()) {
    return;
  }
  auto &packet = writeQueue.front();
  websocket.text(packet->isText());
  auto handler =
      boost::beast::bind_front_handler(&Session::onWrite, shared_from_this());
  if (packet->isText()) {
    websocket.async_write(boost::asio::buffer(packet->getTextBuffer()),
                          std::move(handler));
  } else {
    websocket.async_write(boost::asio::buffer(packet->getBinaryBuffer()),
                          std::move(handler));
  }
}
void Session::fail(boost::beast::error_code errorCode,
                   const std::string &message) const {
  LOG_ERROR("Session {}-{}: {} {}", id, message, errorCode.message(),
            errorCode.value());
}

void Session::close() { onClose(id); }
void Session::send(const std::shared_ptr<Packet> &packet) {
  writeQueue.emplace_back(packet);
  if (writeQueue.size() > 1) {
    return;
  }
  doWrite();
}
const std::string &Session::getId() { return id; }
void Session::send(std::unique_ptr<Notification> &&notification) {
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(notification->serialize());
  send(packet);
}
void Session::send(std::unique_ptr<Request> &&request) {
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(request->serialize());
  send(packet);
}
void Session::send(std::unique_ptr<Response> &&response) {
  std::shared_ptr<Packet> packet =
      std::make_shared<Packet>(response->serialize());
  send(packet);
}
