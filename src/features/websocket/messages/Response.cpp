#include "base_library/features/websocket/messages/Response.h"

#include <rapidjson/document.h>

#include <utility>

#define JSON_RPC_NAME "jsonrpc"
#define JSON_RPC_VALUE "2.0"
#define ERROR_NAME "error"
#define RESULT_NAME "result"
#define ID_NAME "id"

Response::Response(std::string result, std::string error, std::string id)
    : Message(Message::RESPONSE),
      result(std::move(result)),
      error(std::move(error)),
      id(std::move(id)) {}
bool Response::operator==(const Response &rhs) const {
  return result == rhs.result && error == rhs.error && id == rhs.id;
}
bool Response::operator!=(const Response &rhs) const { return !(rhs == *this); }
const std::string &Response::getId() const { return id; }
const std::string &Response::getError() const { return error; }
const std::string &Response::getResult() const { return result; }
std::string Response::getJsonRpc() { return JSON_RPC_VALUE; }
void Response::toJson(const Response &response,
                      rapidjson::Writer<rapidjson::StringBuffer> &writer) {
  writer.StartObject();
  writer.Key(JSON_RPC_NAME);
  writer.String(JSON_RPC_VALUE);
  if (!response.result.empty()) {
    writer.Key(RESULT_NAME);
    writer.String(response.result.c_str());
  }
  if (!response.error.empty()) {
    writer.Key(ERROR_NAME);
    writer.String(response.error.c_str());
  }
  writer.Key(ID_NAME);
  writer.String(response.id.c_str());
  writer.EndObject();
}
std::string Response::toJson() const {
  rapidjson::StringBuffer stringBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
  toJson(*this, writer);
  return stringBuffer.GetString();
}
std::string Response::toJson(const std::vector<Response> &responses) {
  rapidjson::StringBuffer stringBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
  writer.StartArray();
  for (auto &response : responses) {
    toJson(response, writer);
  }
  writer.EndArray();
  return stringBuffer.GetString();
}
std::vector<Response> Response::fromJson(const std::string &serialized) {
  auto func = [](auto &iter) -> Response {
    std::string idTemp;
    std::string errorTemp;
    std::string resultTemp;
    auto found = iter->FindMember(JSON_RPC_NAME);
    if (found == iter->MemberEnd()) {
      throw std::runtime_error("incorrect message format");
    }
    found = iter->FindMember(ID_NAME);
    if (found != iter->MemberEnd()) {
      idTemp = found->value.GetString();
    }
    bool isErrorSet = false;
    found = iter->FindMember(ERROR_NAME);
    if (found != iter->MemberEnd()) {
      errorTemp = found->value.GetString();
      isErrorSet = true;
    }
    found = iter->FindMember(RESULT_NAME);
    if (found != iter->MemberEnd() && !isErrorSet) {
      resultTemp = found->value.GetString();
    } else if (found != iter->MemberEnd() && isErrorSet) {
      throw std::invalid_argument("jsonrpc wrong format only error or result");
    }
    return Response(resultTemp, errorTemp, idTemp);
  };
  std::vector<Response> responses;
  rapidjson::Document document;
  if (document.Parse<0>(serialized.c_str()).HasParseError()) {
    throw std::invalid_argument("json parse error");
  }
  if (document.IsArray()) {
    auto array = document.GetArray();
    for (auto iter = array.begin(); iter < array.end(); iter++) {
      responses.push_back(func(iter));
    }
  } else {
    const rapidjson::Document *temp = &document;
    responses.push_back(func(temp));
  }
  return responses;
}
std::string Response::serialize() { return toJson(); }
void Response::serialize(rapidjson::Writer<rapidjson::StringBuffer> &writer) {
  toJson(*this, writer);
}
