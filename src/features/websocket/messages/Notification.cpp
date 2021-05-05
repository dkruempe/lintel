#include "base_library/features/websocket/messages/Notification.h"

#include <rapidjson/document.h>

#include <utility>

#define JSON_RPC_NAME "jsonrpc"
#define JSON_RPC_VALUE "2.0"
#define METHOD_NAME "method"
#define PARAMS_NAME "params"

bool Notification::operator==(const Notification &rhs) const {
  return m_method == rhs.m_method && m_params == rhs.m_params;
}

bool Notification::operator!=(const Notification &rhs) const {
  return !(rhs == *this);
}

const std::string &Notification::getMethod() const { return m_method; }

const std::string &Notification::getParams() const { return m_params; }

std::string Notification::getJsonRpc() { return JSON_RPC_VALUE; }

Notification::Notification(std::string method, std::string params)
    : Message(Message::NOTIFICATION),
      m_method(std::move(method)),
      m_params(std::move(params)) {}

std::string Notification::toJson() {
  rapidjson::StringBuffer stringBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
  toJson(*this, writer);
  return stringBuffer.GetString();
}

std::string Notification::toJson(std::vector<Notification> &requests) {
  rapidjson::StringBuffer stringBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
  writer.StartArray();
  for (auto &request : requests) {
    toJson(request, writer);
  }
  writer.EndArray();
  return stringBuffer.GetString();
}

void Notification::toJson(Notification &request,
                          rapidjson::Writer<rapidjson::StringBuffer> &writer) {
  writer.StartObject();
  writer.Key(JSON_RPC_NAME);
  writer.String(JSON_RPC_VALUE);
  writer.Key(METHOD_NAME);
  writer.String(request.m_method.c_str());
  if (!request.m_params.empty()) {
    writer.Key(PARAMS_NAME);
    writer.String(request.m_params.c_str());
  }
  writer.EndObject();
}

std::shared_ptr<Notification> Notification::fromJson(jsonType iter) {
  std::string methodTemp;
  std::string paramsTemp;
  auto found = iter->FindMember(JSON_RPC_NAME);
  if (found == iter->MemberEnd()) {
    throw std::runtime_error("incorrect message format");
  }
  found = iter->FindMember(METHOD_NAME);
  if (found != iter->MemberEnd()) {
    methodTemp = found->value.GetString();
  }
  found = iter->FindMember(PARAMS_NAME);
  if (found != iter->MemberEnd()) {
    paramsTemp = found->value.GetString();
  }
  return std::make_shared<Notification>(methodTemp, paramsTemp);
}
std::string Notification::serialize() { return toJson(); }
void Notification::serialize(
    rapidjson::Writer<rapidjson::StringBuffer> &writer) {
  toJson(*this, writer);
}
Message::create_t Notification::createFunctionOf() {
  return
      [&](jsonType json) -> std::shared_ptr<Message> { return fromJson(json); };
}
