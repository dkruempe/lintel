#ifndef CPP_BASE_LIBRARY_SESSION_H
#define CPP_BASE_LIBRARY_SESSION_H

#include <boost/beast.hpp>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "base_library/features/websocket/configuration/WebsocketEntry.h"
#include "base_library/features/websocket/messages/MessageFactory.h"
#include "base_library/features/websocket/messages/Notification.h"
#include "base_library/features/websocket/messages/Request.h"
#include "base_library/features/websocket/messages/Response.h"
#include "base_library/features/websocket/models/Packet.h"
#include "base_library/features/websocket/models/ProcessingRequests.h"

class Controller;
class Session : public std::enable_shared_from_this<Session> {
 private:
  const std::string m_id;
  std::shared_ptr<WebsocketEntry> m_websocketEntry = nullptr;
  boost::beast::websocket::stream<boost::beast::tcp_stream> m_websocket;
  boost::beast::flat_buffer m_readBuffer;
  std::deque<std::shared_ptr<Packet>> m_writeQueue;
  std::function<void(const std::string &)> m_onClose;
  std::map<std::string, std::shared_ptr<Controller>> m_controllers;
  std::map<std::string, std::promise<std::shared_ptr<Response>>>
      m_pendingRequests;
  MessageFactory messageFactory;
  ProcessingRequests processingRequests;
  std::unique_ptr<boost::asio::ip::tcp::resolver> m_resolver = nullptr;
  std::string m_host;
  std::string m_port;

  void doRead();

  void doWrite();

  void onAccept(boost::beast::error_code errorCode);

  void onRead(boost::beast::error_code errorCode, size_t length);

  void onReceive(const std::shared_ptr<Packet> &packet);

  void send(const std::shared_ptr<Packet> &packet);

  void onWrite(boost::beast::error_code errorCode, std::size_t length);

  void onHandshake(boost::beast::error_code ec);

  void onResolve(boost::beast::error_code ec,
                 boost::asio::ip::tcp::resolver::results_type results);

  void fail(boost::beast::error_code errorCode,
            const std::string &message) const;

  void close();

 public:
  void send(const std::shared_ptr<Notification> &notification);
  std::future<std::shared_ptr<Response>> send(
      const std::shared_ptr<Request> &request);
  void send(const std::shared_ptr<Response> &response);

  Session(boost::asio::ip::tcp::socket &&socket,
          std::function<void(const std::string &)> onClose,
          std::map<std::string, std::shared_ptr<Controller>> controllers);

  Session(boost::asio::io_context &context,
          std::function<void(const std::string &)> onClose,
          std::map<std::string, std::shared_ptr<Controller>> controllers,
          std::shared_ptr<WebsocketEntry> connectionEntry);

  ~Session() = default;

  [[nodiscard]] const std::string &getId();

  void run();

  void onConnect(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type ep);
};
#endif  // CPP_BASE_LIBRARY_SESSION_H
