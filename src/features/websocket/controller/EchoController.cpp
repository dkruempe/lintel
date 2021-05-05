#include "base_library/features/websocket/controller/EchoController.h"

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/websocket/controller/EchoRequest.h"
#include "base_library/features/websocket/controller/EchoResponse.h"

std::shared_ptr<Response> EchoController::onReceive(
    const std::shared_ptr<Request>& request) {
  if (request->getMethod() == "EchoRequest") {
    std::shared_ptr<EchoRequest> echoRequest =
        std::static_pointer_cast<EchoRequest>(request);
    LOG_INFO("received echo request: {}", echoRequest->serialize());
    return std::make_shared<EchoResponse>(echoRequest->getId());
  }
  return nullptr;
}
void EchoController::onReceive(
    const std::shared_ptr<Notification>& notification) {}

std::vector<std::string> EchoController::getMethods() {
  return {"EchoRequest"};
}
std::map<std::string, Message::create_t> EchoController::getRequests() {
  Message::create_t func =
      [&](Message::jsonType iter) -> std::shared_ptr<Message> {
    return Request::fromJson(iter);
  };
  return {{std::string(EchoRequest::methodName), func}};
}
std::map<std::string, Message::create_t> EchoController::getResponses() {
  Message::create_t func =
      [&](Message::jsonType iter) -> std::shared_ptr<Message> {
    return Response::fromJson(iter);
  };
  return {{std::string(EchoRequest::methodName), func}};
}
