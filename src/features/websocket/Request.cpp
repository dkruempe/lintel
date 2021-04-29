#include "base_library/features/websocket/Request.h"

#include <utility>

std::string Request::serialize() { return std::string(); }

Request::Request(std::string jsonRPC, std::string method, std::string params,
                 std::string id)
    : Message(),
      m_id(std::move(id)),
      m_params(std::move(params)),
      m_method(std::move(method)),
      m_jsonRPC(std::move(jsonRPC)) {}
const std::string& Request::getId() const { return m_id; }
const std::string& Request::getMethod() const { return m_method; }
const std::string& Request::getJsonRpc() const { return m_jsonRPC; }
std::optional<std::string> Request::getParams() const {
  if (m_params.empty()) {
    return std::nullopt;
  }
  return std::make_optional<std::string>(m_params);
}
std::ostream& operator<<(std::ostream& os, const Request& request) {
  os << static_cast<const Message&>(request) << " id: " << request.m_id
     << " params: " << request.m_params << " method: " << request.m_method
     << " jsonRPC: " << request.m_jsonRPC;
  return os;
}
