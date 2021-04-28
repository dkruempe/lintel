#include "base_library/features/websocket/Request.h"

#include <utility>

std::string Request::serialize() { return std::string(); }

Request::Request(std::string jsonRPC, std::string method, std::string params,
                 std::string id)
    : Message(),
      id(std::move(id)),
      params(std::move(params)),
      method(std::move(method)),
      jsonRPC(std::move(jsonRPC)) {}
const std::string& Request::getId() const { return id; }
const std::string& Request::getMethod() const { return method; }
const std::string& Request::getJsonRpc() const { return jsonRPC; }
std::optional<std::string> Request::getParams() const {
  if (params.empty()) {
    return std::nullopt;
  }
  return std::make_optional<std::string>(params);
}
std::ostream& operator<<(std::ostream& os, const Request& request) {
  os << static_cast<const Message&>(request) << " id: " << request.id
     << " params: " << request.params << " method: " << request.method
     << " jsonRPC: " << request.jsonRPC;
  return os;
}
