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
  // I base variables
  const std::string m_id;
  std::shared_ptr<WebsocketEntry> m_websocketEntry = nullptr;
  std::string m_host;
  std::string m_port;

  // II Websocket Variables
  std::unique_ptr<boost::beast::websocket::stream<boost::beast::tcp_stream>>
      m_websocket;
  boost::beast::flat_buffer m_readBuffer;
  std::unique_ptr<boost::asio::ip::tcp::resolver> m_resolver = nullptr;

  // III others
  std::function<void(const std::string &)> m_onClose;
  std::map<std::string, std::promise<std::shared_ptr<Response>>>
      m_pendingRequests;
  std::deque<std::shared_ptr<Message>> m_writeQueue;
  std::atomic<bool> m_isConnect = false;
  std::atomic<bool> m_exit = false;

  // IV Services Controller/MessageFactory/Processing
  std::map<std::string, std::shared_ptr<Controller>> m_controllers;
  MessageFactory messageFactory;
  ProcessingRequests processingRequests;

  // V Write/Reconnect Thread Variables
  std::mutex m_reconnectMutex;
  std::condition_variable m_reconnectCondition;
  std::thread m_writeThread;
  std::mutex m_writeMutex;
  std::condition_variable m_writeCondition;

  // constants
  const std::size_t m_bufferLimit = 1000;
  const std::size_t m_batchSize = 100;
  std::chrono::seconds m_reconnectInterval = std::chrono::seconds(30);
  std::chrono::seconds m_writeInterval = std::chrono::seconds(5);

  void doRead();

  void onWrite();

  void onAccept(boost::beast::error_code errorCode);

  void onRead(boost::beast::error_code errorCode, size_t length);

  void onReceive(const std::shared_ptr<Packet> &packet);

  void onHandshake(boost::beast::error_code ec);

  void onResolve(boost::beast::error_code ec,
                 boost::asio::ip::tcp::resolver::results_type results);

  void fail(boost::beast::error_code errorCode,
            const std::string &message) const;

  void close();

  void runWrite();

  void doReconnect();

  bool isClient();

 public:
  void send(const std::shared_ptr<Notification> &notification);
  std::future<std::shared_ptr<Response>> send(
      const std::shared_ptr<Request> &request);
  void send(const std::shared_ptr<Response> &response);
  std::vector<std::future<std::shared_ptr<Response>>> send(
      const std::vector<std::shared_ptr<Message>> &messages);

  Session(boost::asio::ip::tcp::socket &&socket,
          std::function<void(const std::string &)> onClose,
          std::map<std::string, std::shared_ptr<Controller>> controllers);

  Session(boost::asio::io_context &context,
          std::function<void(const std::string &)> onClose,
          std::map<std::string, std::shared_ptr<Controller>> controllers,
          std::shared_ptr<WebsocketEntry> connectionEntry);

  ~Session();

  [[nodiscard]] const std::string &getId();

  void run();

  void onConnect(
      boost::beast::error_code ec,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type ep);

  bool isConnected();

  void onShutdown();
};
#endif  // CPP_BASE_LIBRARY_SESSION_H
