#include "base_library/features/websocket/MessageFactory.h"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include "base_library/core/services/LoggerService.h"
#include "base_library/features/websocket/Notification.h"
#include "base_library/features/websocket/Request.h"
#include "base_library/features/websocket/Response.h"

#define RPC_JSONRPC "jsonrpc"
#define RPC_ID "id"
#define RPC_RESULT "result"
#define RPC_ERROR "error"
#define RPC_METHOD "method"
#define RPC_PARAMS "params"

std::vector<std::shared_ptr<Message>> MessageFactory::generate(
    const std::string& message) {
  std::vector<std::shared_ptr<Message>> messages;
  rapidjson::Document document;
  if (document.Parse<0>(message.c_str()).HasParseError()) {
    throw std::invalid_argument("json parse error");
  }
  auto func = [&](auto& iter) {
    // is valid ?
    auto foundJsonRPC = iter->FindMember(RPC_JSONRPC);
    if (foundJsonRPC == iter->MemberEnd()) {
      // TODO replace with working exception handling
      LOG_ERROR("invalid message received {}", iter->GetString());
      return;
    }
    std::string jsonrpc = foundJsonRPC->value.GetString();
    if (jsonrpc != "2.0") {
      LOG_ERROR("invalid message received {}", iter->GetString());
      return;
    }
    // is Notification ?
    auto foundId = iter->FindMember(RPC_ID);
    auto foundMethod = iter->FindMember(RPC_METHOD);
    auto foundParameter = iter->FindMember(RPC_PARAMS);
    if (foundId == iter->MemberEnd() && foundMethod != iter->MemberEnd()) {
      auto notification = std::make_shared<Notification>();
      messages.push_back(std::move(notification));
      return;
    }

    // is Request ?
    if (foundId != iter->MemberEnd() && foundMethod != iter->MemberEnd()) {
      std::string params;
      if (foundParameter != iter->MemberEnd()) {
        // params = foundParameter->value
      }
      auto request =
          std::make_shared<Request>(jsonrpc, foundMethod->value.GetString(),
                                    params, foundId->value.GetString());
      messages.push_back(std::move(request));
      return;
    }

    auto foundResult = iter->FindMember(RPC_RESULT);
    auto foundError = iter->FindMember(RPC_ERROR);
    if (foundId != iter->MemberEnd() && foundMethod == iter->MemberEnd() &&
        foundParameter == iter->MemberEnd() &&
        (foundResult != iter->MemberEnd() || foundError != iter->MemberEnd())) {
      auto response = std::make_shared<Response>();
      messages.push_back(std::move(response));
      return;
    }
    LOG_ERROR("invalid message received {}", iter->GetString());
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
  return messages;
}
