#include <httplib.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"
#include "lintel/features/base/controller/EventBusDtos.h"
#include "lintel/features/http/controllers/EventBusApi.h"
#include "lintel/features/http/service/HttpClientHelper.h"
#include "lintel/features/http/service/HttpStatusCodes.h"
#include "lintel/features/http/service/HttpUnauthorizedException.h"

namespace lintel {
namespace {
  /** @return the JSON body of a publish request */
  std::string publishBodyOf(const std::string &topic,
    const std::string &type,
    const std::string &payload,
    const std::string &priority,
    bool toSubscribers)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);
    writer.StartObject();
    writer.String("PAYLOAD");
    writer.String(payload.c_str());
    writer.String("PRIORITY");
    writer.String(priority.c_str());
    writer.String("TARGET");
    writer.String(toSubscribers ? "SUBSCRIBERS" : "BUS");
    writer.String("TOPIC");
    writer.String(topic.c_str());
    writer.String("TYPE");
    writer.String(type.c_str());
    writer.EndObject();
    return std::string(buffer.GetString());
  }

  /** @return the JSON body of a subscribe/unsubscribe request */
  std::string subscriptionBodyOf(const std::string &topic, const std::string &subscriber)
  {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);
    writer.StartObject();
    writer.String("SUBSCRIBER");
    writer.String(subscriber.c_str());
    writer.String("TOPIC");
    writer.String(topic.c_str());
    writer.EndObject();
    return std::string(buffer.GetString());
  }

  /** @param result a completed request
   * @return true if the response may be parsed, false if the request was rejected
   * @throws HttpUnauthorizedException if the token was missing or rejected */
  bool isSuccessful(const httplib::Result &result)
  {
    if (!HttpClientHelper::hasResponse(result)) { return false; }
    HttpStatusCodes const status(result->status);
    switch (status) {
    case HttpStatusCodes::Unauthorized:
      throw HttpUnauthorizedException();
    case HttpStatusCodes::OK:
      return true;
    default:
      LOG_ERROR("event bus request rejected with status {}", status.getCode());
      return false;
    }
  }

  /** @param body a JSON object
   * @param key member name
   * @return the boolean member, nullopt when it is missing or mistyped */
  std::optional<bool> boolOf(const std::string &body, const char *key)
  {
    rapidjson::Document document;
    document.Parse(body.c_str());
    if (document.HasParseError() || !document.IsObject()) { return std::nullopt; }
    bool value = false;
    if (!json::getBool(document, key, value)) { return std::nullopt; }
    return value;
  }
}// namespace

EventBusApi::EventBusApi(const std::shared_ptr<ClientProvider> &clientProvider) : m_client(clientProvider->provide()) {}

std::vector<EventBusDto> EventBusApi::allOf()
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/eventBus/buses", headers);
  if (!isSuccessful(result)) { return {}; }
  EventBusesDto eventBusesDto;
  try {
    eventBusesDto.deserialize(result->body);
  } catch (const std::exception &exception) {
    LOG_ERROR("{}", exception.what());
    return {};
  }
  return eventBusesDto.getEventBusDtos();
}

std::optional<EventBusDto> EventBusApi::of(const std::string &name)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/eventBus/bus/" + name, headers);
  if (!isSuccessful(result)) { return std::nullopt; }
  EventBusDto eventBusDto;
  try {
    eventBusDto.deserialize(result->body);
  } catch (const std::exception &exception) {
    LOG_ERROR("{}", exception.what());
    return std::nullopt;
  }
  return eventBusDto;
}

std::optional<bool> EventBusApi::publishOf(const EventPublishRequest &request)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const std::string body =
    publishBodyOf(request.topic, request.type, request.payload, request.priority, request.toSubscribers);
  const httplib::Result &result = m_client->post("/eventBus/publish/" + request.bus, headers, body, "application/json");
  if (!isSuccessful(result)) { return std::nullopt; }
  return boolOf(result->body, "PUBLISHED");
}

std::optional<bool> EventBusApi::subscribeOf(const EventSubscription &subscription)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const std::string body = subscriptionBodyOf(subscription.topic, subscription.subscriber);
  const httplib::Result &result =
    m_client->post("/eventBus/subscribe/" + subscription.bus, headers, body, "application/json");
  if (!isSuccessful(result)) { return std::nullopt; }
  return boolOf(result->body, "SUBSCRIBED");
}

std::optional<bool> EventBusApi::unsubscribeOf(const EventSubscription &subscription)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const std::string body = subscriptionBodyOf(subscription.topic, subscription.subscriber);
  const httplib::Result &result =
    m_client->deletes("/eventBus/subscribe/" + subscription.bus, headers, body, "application/json");
  if (!isSuccessful(result)) { return std::nullopt; }
  return boolOf(result->body, "UNSUBSCRIBED");
}

std::optional<std::size_t> EventBusApi::depthOf(const std::string &name, const std::string &subscriber)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->get("/eventBus/depth/" + name + "/" + subscriber, headers);
  if (!isSuccessful(result)) { return std::nullopt; }
  rapidjson::Document document;
  document.Parse(result->body.c_str());
  if (document.HasParseError() || !document.IsObject()) { return std::nullopt; }
  std::uint64_t depth = 0;
  if (!json::getUint64(document, "DEPTH", depth)) { return std::nullopt; }
  return static_cast<std::size_t>(depth);
}

std::optional<bool> EventBusApi::resetOf(const std::string &name)
{
  httplib::Headers headers{};
  headers.insert({ "Content-Type", "application/json" });
  const httplib::Result &result = m_client->post("/eventBus/reset/" + name, headers, "", "application/json");
  if (!isSuccessful(result)) { return std::nullopt; }
  return boolOf(result->body, "RESET");
}
}// namespace lintel
