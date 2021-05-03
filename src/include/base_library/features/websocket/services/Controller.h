#ifndef CPP_BASE_LIBRARY_CONTROLLER_H
#define CPP_BASE_LIBRARY_CONTROLLER_H

#include <memory>

#include "Session.h"
#include "base_library/features/websocket/messages/Notification.h"
#include "base_library/features/websocket/messages/Request.h"
#include "base_library/features/websocket/messages/Response.h"

class Controller {
 public:
  virtual void onReceive(Session &session,
                         const std::shared_ptr<Request> &request) = 0;
  virtual void onReceive(Session &session,
                         const std::shared_ptr<Response> &response) = 0;
  virtual void onReceive(Session &session,
                         const std::shared_ptr<Notification> &notification) = 0;
};

#endif  // CPP_BASE_LIBRARY_CONTROLLER_H
