#ifndef CPP_BASE_LIBRARY_ECHOCONTROLLER_H
#define CPP_BASE_LIBRARY_ECHOCONTROLLER_H

#include "base_library/features/websocket/services/Controller.h"

class EchoController : public Controller {
 public:
  std::shared_ptr<Response> onReceive(
      const std::shared_ptr<Request> &request) override;

  void onReceive(const std::shared_ptr<Notification> &notification) override;

  std::vector<std::string> getMethods() override;

  std::map<std::string, Message::create_t> getRequests() override;

  std::map<std::string, Message::create_t> getResponses() override;
};

#endif  // CPP_BASE_LIBRARY_ECHOCONTROLLER_H
