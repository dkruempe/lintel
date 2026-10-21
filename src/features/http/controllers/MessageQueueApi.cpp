#include <httplib.h>

#include "lintel/features/http/controllers/MessageQueueApi.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/features/base/controller/MessageQueueDtos.h"
#include "lintel/features/http/service/HttpClientHelper.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"

namespace lintel {
MesssageQueueApi::MesssageQueueApi(const std::shared_ptr<ClientProvider> &clientProvider)
  : m_client(clientProvider->provide())
{}

std::vector<MessageQueueDto> MesssageQueueApi::allOf(const MessageQueueFilter &filter)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const auto &result = m_client->get("/messageQueue/" + filter.processName + "/" + filter.queueName, headers);
  if (!HttpClientHelper::hasResponse(result)) { return {}; }
  HttpStatusCodes const status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    return {};
  }
  MessageQueueDtos messageQueueDtos;
  try {
    messageQueueDtos.deserialize(result->body);
  } catch (std::exception &exception) {
    LOG_ERROR("{}", exception.what());
    return {};
  }
  return messageQueueDtos.getMessageQueueDtos();
}

std::optional<std::int32_t> MesssageQueueApi::purgeOf(const std::string &messageQueueName)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const auto &result = m_client->post("/messageQueue/purge/" + messageQueueName, headers, "", "application/json");
  if (!HttpClientHelper::hasResponse(result)) { return std::nullopt; }
  HttpStatusCodes const status(result->status);
  switch (status) {
  case HttpStatusCodes::Unauthorized:
    throw HttpUnauthorizedException();
  case HttpStatusCodes::OK:
    break;
  default:
    return std::nullopt;
  }
  MessageQueuePurgeDto purgeResult;
  try {
    purgeResult.deserialize(result->body);
  } catch (std::exception &exception) {
    LOG_ERROR("{}", exception.what());
    return std::nullopt;
  }
  return purgeResult.getPurged();
}
}// namespace lintel