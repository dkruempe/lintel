#include "base_library/features/websocket/messages/Request.h"

#include <rapidjson/document.h>

#include <utility>

#define JSON_RPC_NAME "jsonrpc"
#define JSON_RPC_VALUE "2.0"
#define METHOD_NAME "method"
#define PARAMS_NAME "params"
#define ID_NAME "id"

bool Request::operator==(const Request &rhs) const {
  return m_method == rhs.m_method && m_params == rhs.m_params &&
         m_id == rhs.m_id;
}

bool Request::operator!=(const Request &rhs) const { return !(rhs == *this); }

const std::string &Request::getId() const { return m_id; }

const std::string &Request::getMethod() const { return m_method; }

const std::string &Request::getParams() const { return m_params; }

std::string Request::getJsonRpc() { return JSON_RPC_VALUE; }

Request::Request(std::string method, std::string params, std::string id)
    : Message(Message::REQUEST),
      m_method(std::move(method)),
      m_params(std::move(params)),
      m_id(std::move(id)) {}

std::string Request::toJson() {
  rapidjson::StringBuffer stringBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
  toJson(*this, writer);
  return stringBuffer.GetString();
}

std::string Request::toJson(std::vector<Request> &requests) {
  rapidjson::StringBuffer stringBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
  writer.StartArray();
  for (auto &request : requests) {
    toJson(request, writer);
  }
  writer.EndArray();
  return stringBuffer.GetString();
}

void Request::toJson(Request &request,
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
  writer.Key(ID_NAME);
  writer.String(request.m_id.c_str());
  writer.EndObject();
}
std::shared_ptr<Request> Request::fromJson(Message::jsonType iter) {
  std::string idTemp;
  std::string methodTemp;
  std::string paramsTemp;
  auto found = iter->FindMember(JSON_RPC_NAME);
  if (found == iter->MemberEnd()) {
    return nullptr;
  }
  found = iter->FindMember(ID_NAME);
  if (found != iter->MemberEnd()) {
    idTemp = found->value.GetString();
  }
  found = iter->FindMember(METHOD_NAME);
  if (found != iter->MemberEnd()) {
    methodTemp = found->value.GetString();
  }
  found = iter->FindMember(PARAMS_NAME);
  if (found != iter->MemberEnd()) {
    paramsTemp = found->value.GetString();
  }
  return std::make_shared<Request>(methodTemp, paramsTemp, idTemp);
}
std::string Request::serialize() { return toJson(); }
void Request::serialize(rapidjson::Writer<rapidjson::StringBuffer> &writer) {
  return toJson(*this, writer);
}
Message::create_t Request::createFunctionOf() {
  return
      [&](jsonType json) -> std::shared_ptr<Message> { return fromJson(json); };
}
