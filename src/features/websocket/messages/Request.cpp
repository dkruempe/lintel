#include "base_library/features/websocket/messages/Request.h"

#include <rapidjson/document.h>

#include <utility>

#define JSON_RPC_NAME "jsonrpc"
#define JSON_RPC_VALUE "2.0"
#define METHOD_NAME "method"
#define PARAMS_NAME "params"
#define ID_NAME "id"

bool Request::operator==(const Request &rhs) const {
  return m_method == rhs.m_method && m_params == rhs.m_params && m_id == rhs.m_id;
}

bool Request::operator!=(const Request &rhs) const { return !(rhs == *this); }

const std::string &Request::getId() const { return m_id; }

const std::string &Request::getMethod() const { return m_method; }

const std::string &Request::getParams() const { return m_params; }

std::string Request::getJsonRpc() { return JSON_RPC_VALUE; }

Request::Request(std::string jsonRPC, std::string method, std::string params, std::string id)
    : Message(Message::REQUEST),
      m_jsonRPC(std::move(jsonRPC)),
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

std::vector<Request> Request::fromJson(const std::string &serialized) {
  // generic lambda function for reducing duplicate code
  auto func = [](auto &iter) -> Request {
    std::string idTemp;
    std::string methodTemp;
    std::string paramsTemp;
    auto found = iter->FindMember(JSON_RPC_NAME);
    if (found == iter->MemberEnd()) {
      throw std::runtime_error("incorrect message format");
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
    return Request(JSON_RPC_NAME, methodTemp, paramsTemp, idTemp);
  };
  // parse json array or normal json object
  std::vector<Request> requests;
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
std::string Request::serialize() {
  return toJson();
}
