#include "base_library/features/websocket/messages/MessageFactory.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#define RPC_JSONRPC "jsonrpc"
#define RPC_JSONRPC_VALUE "2.0"
#define RPC_ID "id"
#define RPC_RESULT "result"
#define RPC_ERROR "error"
#define RPC_METHOD "method"
#define RPC_PARAMS "params"

#include "base_library/core/services/LoggerService.h"
void MessageFactory::registerRequest(
    const std::string& method, const MessageFactory::create_t& createFunction) {
  m_requests.insert({method, createFunction});
}
void MessageFactory::registerResponse(
    const std::string& method, const MessageFactory::create_t& createFunction) {
  m_responses.insert({method, createFunction});
}
MessageContainer MessageFactory::generate(
    const std::string& message, ProcessingRequests& processingRequests) {
  MessageContainer container;
  rapidjson::Document document;
  if (document.Parse<0>(message.c_str()).HasParseError()) {
    container.addError(createErrorMessage(ErrorCode::PARSE_ERROR,
                                          "parse error of send messages"));
    return container;
  }
  auto func = [&](auto* iter) {
    auto hasMethod = iter->FindMember(RPC_METHOD);
    auto hasResult = iter->FindMember(RPC_RESULT);
    auto hasError = iter->FindMember(RPC_ERROR);
    auto hasId = iter->FindMember(RPC_ID);
    auto hasJsonRPC = iter->FindMember(RPC_JSONRPC);
    auto hasParams = iter->FindMember(RPC_PARAMS);
    // get id or check for correctness
    std::string id;
    if (hasId != iter->MemberEnd() && hasId->value.IsString()) {
      id = hasId->value.GetString();
    } else if (hasId != iter->MemberEnd() && !hasId->value.IsString()) {
      container.addError(
          createErrorMessage(ErrorCode::PARSE_ERROR, "id is not a string"));
      LOG_ERROR("id is not string for parsed message");
      return;
    }
    // check if jsonrpc is correct
    if (hasJsonRPC == iter->MemberEnd() || !hasJsonRPC->value.IsString() ||
        std::strcmp(hasJsonRPC->value.GetString(), RPC_JSONRPC_VALUE) != 0) {
      container.addError(createErrorMessage(ErrorCode::PARSE_ERROR,
                                            "jsonrpc is wrong or not set"));
      return;
    }
    std::string method;
    if (hasMethod != iter->MemberEnd() && hasMethod->value.IsString()) {
      method = hasMethod->value.GetString();
    } else if (hasMethod != iter->MemberEnd()) {
      container.addError(
          createErrorMessage(ErrorCode::PARSE_ERROR, "method is not string"));
      LOG_ERROR("method available but not a string");
      return;
    }
    // check if request has correct structure
    if (!method.empty() &&
        (hasResult != iter->MemberEnd() || hasError != iter->MemberEnd())) {
      container.addError(createErrorMessage(ErrorCode::PARSE_ERROR,
                                            "has method and result or error"));
      LOG_ERROR("has method but also result or error");
      return;
    }
    if ((hasResult != iter->MemberEnd() || hasError != iter->MemberEnd()) &&
        hasParams != iter->MemberEnd()) {
      container.addError(createErrorMessage(ErrorCode::PARSE_ERROR,
                                            "is response but has also params"));
      LOG_ERROR("has result or error but also params");
      return;
    }
    // check if request has not to much or to less members
    if (!method.empty() &&
        (iter->MemberCount() > 4 || iter->MemberCount() < 2)) {
      container.addError(createErrorMessage(
          ErrorCode::PARSE_ERROR, "requests has to much or to less members"));
      LOG_ERROR("request has more or less members than defined");
      return;
    }
    // check if response has not to much or to less members
    if ((hasResult != iter->MemberEnd() || hasError != iter->MemberEnd()) &&
        iter->MemberCount() != 3) {
      container.addError(createErrorMessage(
          ErrorCode::PARSE_ERROR, "response has to less or to much members"));
      LOG_ERROR("response has more or less members than defined");
      return;
    }
    if (!method.empty()) {
      try {
        container.add(m_requests.at(method)(iter));
      } catch (std::out_of_range& e) {
        container.addError(createErrorMessage(ErrorCode::METHOD_NOT_FOUND,
                                              "method not available", method));
        LOG_ERROR("method not available for >{}<", method);
      }
    } else if (!id.empty()) {
      try {
        auto methodOpt = processingRequests.acknowledgeOf(id);
        if (methodOpt.has_value()) {
          container.add(m_responses.at(methodOpt.value())(iter));
        } else {
          container.addError(createErrorMessage(
              ErrorCode::METHOD_NOT_FOUND,
              "no processing response with the mentioned id", id));
          LOG_ERROR("no processing response with the mentioned id >{}<", id);
        }
      } catch (std::out_of_range& e) {
        container.addError(
            createErrorMessage(ErrorCode::METHOD_NOT_FOUND,
                               "no processing response with the mentioned id"));
        LOG_ERROR("no processing response with the mentioned id >{}<", id);
      }
    }
  };
  if (document.IsArray()) {
    auto array = document.GetArray();
    for (auto iter = array.begin(); iter < array.end(); iter++) {
      func(iter);
    }
  } else {
    func(&document);
  }
  return container;
}
std::shared_ptr<ErrorMessage> MessageFactory::createErrorMessage(
    ErrorCode errorCode, std::string message, std::string id) {
  return std::make_shared<ErrorMessage>(errorCode, message, id);
}
