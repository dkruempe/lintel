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

Notification::Notification(std::string jsonRPC, std::string method,
                           std::string params)
    : Message(Message::NOTIFICATION),
      m_jsonRPC(std::move(jsonRPC)),
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

std::vector<Notification> Notification::fromJson(
    const std::string &serialized) {
  // generic lambda function for reducing duplicate code
  auto func = [](auto &iter) -> Notification {
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
    return Notification(JSON_RPC_NAME, methodTemp, paramsTemp);
  };
  // parse json array or normal json object
  std::vector<Notification> requests;
  rapidjson::Document document;
  if (document.Parse<0>(serialized.c_str()).HasParseError()) {
    throw std::invalid_argument("json parse error");
  }
  if (document.IsArray()) {
    auto array = document.GetArray();
    for (auto iter = array.begin(); iter < array.end(); iter++) {
      requests.push_back(func(iter));
    }
  } else {
    const rapidjson::Document *temp = &document;
    requests.push_back(func(temp));
  }
  return requests;
}
std::string Notification::serialize() { return toJson(); }
