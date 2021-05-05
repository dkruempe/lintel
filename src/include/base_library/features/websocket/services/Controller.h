#ifndef CPP_BASE_LIBRARY_CONTROLLER_H
#define CPP_BASE_LIBRARY_CONTROLLER_H

#include <memory>
#include <string>
#include <vector>

#include "base_library/features/websocket/messages/Notification.h"
#include "base_library/features/websocket/messages/Request.h"
#include "base_library/features/websocket/messages/Response.h"

class Controller {
 public:
  virtual std::shared_ptr<Response> onReceive(
      const std::shared_ptr<Request> &request) = 0;

  virtual void onReceive(const std::shared_ptr<Notification> &notification) = 0;

  virtual std::vector<std::string> getMethods() = 0;

  virtual ~Controller() = default;
};

#endif  // CPP_BASE_LIBRARY_CONTROLLER_H
