#ifndef CPP_BASE_LIBRARY_SESSION_H
#define CPP_BASE_LIBRARY_SESSION_H

#include <boost/beast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "base_library/features/websocket/messages/Notification.h"
#include "base_library/features/websocket/messages/Request.h"
#include "base_library/features/websocket/messages/Response.h"
#include "base_library/features/websocket/models/Packet.h"

class Controller;
class Session : public std::enable_shared_from_this<Session> {
 private:
  const std::string m_id;
  boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_websocket;
  boost::beast::flat_buffer m_readBuffer;
  std::deque<std::shared_ptr<Packet>> m_writeQueue;
  std::function<void(const std::string &)> m_onClose;
  std::vector<std::shared_ptr<Controller>> m_controllers;

  void doRead();

  void doWrite();

  void onAccept(boost::beast::error_code errorCode);

  void onRead(boost::beast::error_code errorCode, size_t length);

  void onReceive(const std::shared_ptr<Packet> &packet);

  void send(const std::shared_ptr<Packet> &packet);

  void onWrite(boost::beast::error_code errorCode, std::size_t length);

  void fail(boost::beast::error_code errorCode,
            const std::string &message) const;

  void close();

 public:
  void send(const std::shared_ptr<Notification> &notification);
  void send(const std::shared_ptr<Request> &request);
  void send(const std::shared_ptr<Response> &response);

  explicit Session(boost::asio::ip::tcp::socket &&socket,
                   std::function<void(const std::string &)> onClose,
                   std::vector<std::shared_ptr<Controller>> controllers);

  ~Session() = default;

  [[nodiscard]] const std::string &getId();

  void run();
};
#endif  // CPP_BASE_LIBRARY_SESSION_H
