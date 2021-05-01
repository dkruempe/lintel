#include "base_library/features/websocket/messages/MessageFactory.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/websocket/messages/ErrorMessage.h"
#include "base_library/features/websocket/messages/Notification.h"
#include "base_library/features/websocket/messages/Request.h"
#include "base_library/features/websocket/messages/Response.h"

#define RPC_JSONRPC "jsonrpc"
#define RPC_ID "id"
#define RPC_RESULT "result"
#define RPC_ERROR "error"
#define RPC_METHOD "method"
#define RPC_PARAMS "params"

MessageContainer MessageFactory::generate(const std::string& message) {
  MessageContainer messageContainer;
  rapidjson::Document document;
  if (document.Parse<0>(message.c_str()).HasParseError()) {
    throw std::invalid_argument("json parse error");
  }
  auto toString = [&](auto& iter) -> std::string {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    iter.Accept(writer);
    return buffer.GetString();
  };
  auto func = [&](auto& iter) {
    // is valid ?
    auto foundId = iter->FindMember(RPC_ID);
    auto foundJsonRPC = iter->FindMember(RPC_JSONRPC);
    if (foundJsonRPC == iter->MemberEnd()) {
      std::shared_ptr<ErrorMessage> errorMessage;
      if (foundId != iter->MemberEnd()) {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::PARSE_ERROR, "jsonrpc definition missing",
            foundId->value.GetString());
      } else {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::PARSE_ERROR, "jsonrpc definition missing", "");
      }
      messageContainer.addError(errorMessage);
      LOG_ERROR("invalid message received {}", toString(*iter));
      return;
    }
    if (!foundJsonRPC->value.IsString()) {
      std::shared_ptr<ErrorMessage> errorMessage;
      if (foundId != iter->MemberEnd()) {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::PARSE_ERROR, "jsonrpc is not string",
            foundId->value.GetString());
      } else {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::PARSE_ERROR, "jsonrpc is not string", "");
      }
      messageContainer.addError(errorMessage);
      LOG_ERROR("invalid message received {}", toString(*iter));
      return;
    }
    std::string jsonrpc = foundJsonRPC->value.GetString();
    if (jsonrpc != "2.0") {
      std::shared_ptr<ErrorMessage> errorMessage;
      if (foundId != iter->MemberEnd()) {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::INVALID_PARAMETERS, "jsonrpc is not of version 2.0",
            foundId->value.GetString());
      } else {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::INVALID_PARAMETERS, "jsonrpc is not of version 2.0", "");
      }
      messageContainer.addError(errorMessage);
      LOG_ERROR("invalid message received {}", toString(*iter));
      return;
    }
    // is Notification ?
    auto foundMethod = iter->FindMember(RPC_METHOD);
    auto foundParameter = iter->FindMember(RPC_PARAMS);
    std::string params;
    if (foundParameter != iter->MemberEnd()) {
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      foundParameter->value.Accept(writer);
      params = buffer.GetString();
    }
    if (!foundId->value.IsString() || !foundMethod->value.IsString()) {
      std::shared_ptr<ErrorMessage> errorMessage;
      if (foundId->value.IsString() && foundId != iter->MemberEnd()) {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::PARSE_ERROR, "method or id are not a string",
            foundId->value.GetString());
      } else {
        errorMessage = std::make_shared<ErrorMessage>(
            ErrorCode::PARSE_ERROR, "method or id are not a string", "");
      }
      messageContainer.addError(errorMessage);
      LOG_ERROR("invalid message received {}", toString(*iter));
      return;
    }
    if (foundId == iter->MemberEnd() && foundMethod != iter->MemberEnd()) {
      auto notification = std::make_shared<Notification>(
          jsonrpc, foundMethod->value.GetString(), params);
      messageContainer.add(std::move(notification));
      return;
    }

    // is Request ?
    if (foundId != iter->MemberEnd() && foundMethod != iter->MemberEnd()) {
      auto request =
          std::make_shared<Request>(jsonrpc, foundMethod->value.GetString(),
                                    params, foundId->value.GetString());
      messageContainer.add(std::move(request));
      return;
    }

    auto foundResult = iter->FindMember(RPC_RESULT);
    auto foundError = iter->FindMember(RPC_ERROR);
    if (foundId != iter->MemberEnd() && foundMethod == iter->MemberEnd() &&
        foundParameter == iter->MemberEnd() &&
        (foundResult != iter->MemberEnd() || foundError != iter->MemberEnd())) {
      std::string result;
      std::string error;
      if (foundResult != iter->MemberEnd()) {
        result = toString(foundResult->value);
      }
      if (foundError != iter->MemberEnd()) {
        error = toString(foundError->value);
      }
      auto response = std::make_shared<Response>(jsonrpc, result, error,
                                                 foundId->value.GetString());
      messageContainer.add(std::move(response));
      return;
    }
    std::shared_ptr<ErrorMessage> errorMessage;
    if (foundId != iter->MemberEnd()) {
      errorMessage = std::make_shared<ErrorMessage>(ErrorCode::PARSE_ERROR,
                                                    "invalid structure",
                                                    foundId->value.GetString());
    } else {
      errorMessage = std::make_shared<ErrorMessage>(ErrorCode::PARSE_ERROR,
                                                    "invalid structure", "");
    }
    messageContainer.addError(errorMessage);
    LOG_ERROR("invalid message received {}", toString(*iter));
  };
  if (document.IsArray()) {
    auto array = document.GetArray();
    for (auto iter = array.begin(); iter < array.end(); iter++) {
      func(iter);
    }
  } else {
    const rapidjson::Document* temp = &document;
    func(temp);
  }
  return messageContainer;
}
