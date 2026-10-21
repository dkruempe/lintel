#include "lintel/features/http/controllers/EventBusController.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"
#include "lintel/features/base/events/Event.h"
#include "lintel/features/http/service/HttpStatusCodes.h"

namespace lintel {
namespace {
  /** Longest topic/type/subscriber name that fits into an Event buffer. */
  constexpr std::size_t kMaxNameLength = Event::NAME_SIZE - 1;
  /** Longest payload that fits into an Event buffer. */
  constexpr std::size_t kMaxPayloadLength = Event::CONTENT_SIZE;

  /** @param name a topic, type or subscriber name
   * @return true if the name is not empty and fits into an Event */
  bool isValidName(const std::string &name) { return !name.empty() && name.size() <= kMaxNameLength; }

  /** @param payload a payload
   * @return true if the payload fits into an Event without truncation */
  bool isValidPayload(const std::string &payload) { return payload.size() <= kMaxPayloadLength; }

  /** @param value a string
   * @return the string with all ASCII letters converted to lower case */
  std::string lowerOf(const std::string &value)
  {
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char character) {
      return static_cast<char>(std::tolower(character));
    });
    return lower;
  }

  /** @param value priority name, case-insensitive
   * @return the priority, empty for an unknown name */
  std::optional<EventPriority> priorityOf(const std::string &value)
  {
    const std::string lower = lowerOf(value);
    if (lower == "critical") { return EventPriority::Critical; }
    if (lower == "high") { return EventPriority::High; }
    if (lower == "normal") { return EventPriority::Normal; }
    if (lower == "low") { return EventPriority::Low; }
    return std::nullopt;
  }

  /** @param priority a priority
   * @return the name used on the wire */
  const char *priorityNameOf(EventPriority priority)
  {
    switch (priority) {
    case EventPriority::Critical:
      return "Critical";
    case EventPriority::High:
      return "High";
    case EventPriority::Normal:
      return "Normal";
    case EventPriority::Low:
      return "Low";
    }
    return "Normal";
  }

  /** @param value target name, case-insensitive
   * @return true to broadcast to the subscribers, false for the point-to-point
   *         queue, empty for an unknown name */
  std::optional<bool> toSubscribersOf(const std::string &value)
  {
    const std::string lower = lowerOf(value);
    if (lower == "subscribers") { return true; }
    if (lower == "bus") { return false; }
    return std::nullopt;
  }

  /** @param match a matched path segment of the route pattern
   * @return the segment as string */
  std::string segmentOf(const std::ssub_match &match) { return match.str(); }

  /** @return the current wall clock time as seconds since the epoch */
  std::int64_t timestampNow()
  {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
      .count();
  }

  /** @param body parsed JSON request body
   * @param key member name
   * @param out receives the string value
   * @return true if the member is absent or a string, false on a type mismatch */
  bool optionalStringOf(const rapidjson::Document &body, const char *key, std::string &out)
  {
    if (json::member(body, key) == nullptr) { return true; }
    return json::getString(body, key, out);
  }
}// namespace

EventBusController::EventBusController(const std::shared_ptr<IAuthService> &authService,
  std::shared_ptr<EventBusService> eventBusService)
  : Controller(authService), m_eventBusService(std::move(eventBusService)), m_adminGroup("Admin-EventBus", {}, true),
    m_userGroup("User-EventBus", {}, true)
{
  add(m_adminGroup);
  add(m_userGroup);
}

bool EventBusController::authorize(httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user,
  Access access) const
{
  if (!user.has_value()) {
    // Same answer for "no token" and "wrong group": telling them apart would
    // confirm to an anonymous caller that the endpoint exists.
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return false;
  }
  const bool isAdmin = user->m_user.has(m_adminGroup);
  const bool permitted = access == Access::Administer ? isAdmin : (isAdmin || user->m_user.has(m_userGroup));
  if (!permitted) {
    response.status = HttpStatusCodes::Unauthorized;
    response.set_content("", contentType.getName());
    return false;
  }
  if (contentType != ContentType::ApplicationJson) {
    response.status = HttpStatusCodes::Forbidden;
    response.set_content("", contentType.getName());
    return false;
  }
  return true;
}

std::optional<std::shared_ptr<IEventBus>>
  EventBusController::busOf(httplib::Response &response, const ContentType &contentType, const std::string &name)
{
  const std::vector<std::string> names = m_eventBusService->namesOf();
  if (std::find(names.begin(), names.end(), name) == names.end()) {
    response.status = HttpStatusCodes::NotFound;
    response.set_content("", contentType.getName());
    return std::nullopt;
  }
  return m_eventBusService->of(name);
}

EventBusDto EventBusController::describeOf(const std::string &name) const
{
  EventBusDto eventBusDto(name);
  try {
    eventBusDto = EventBusDto(name, m_eventBusService->configOf(name));
  } catch (const std::exception &exception) {
    LOG_WARN("event bus '{}' has no usable configuration: {}", name, exception.what());
    return eventBusDto;
  }
  try {
    const std::shared_ptr<IEventBus> bus = m_eventBusService->of(name);
    eventBusDto.markAvailable(EventBusCounters{ bus->droppedEventsOf(),
      bus->topicCount(),
      bus->subscriberCount(),
      m_eventBusService->busQueueDepthOf(name),
      bus->sequenceOf() });
  } catch (const std::exception &exception) {
    // a bus whose segment is missing stays listed, just without live counters
    LOG_WARN("event bus '{}' cannot be opened: {}", name, exception.what());
  }
  return eventBusDto;
}

void EventBusController::busesOfGet(const httplib::Request & /*request*/,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!authorize(response, contentType, user, Access::Read)) { return; }
  const std::vector<std::string> names = m_eventBusService->namesOf();
  std::vector<EventBusDto> eventBusDtos;
  eventBusDtos.reserve(names.size());
  for (const auto &name : names) { eventBusDtos.push_back(describeOf(name)); }
  const EventBusesDto eventBusesDto(std::move(eventBusDtos));
  response.set_content(eventBusesDto.JsonSerializable::serialize(), contentType.getName());
}

void EventBusController::busOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!authorize(response, contentType, user, Access::Read)) { return; }
  if (!busOf(response, contentType, segmentOf(request.matches[1])).has_value()) { return; }
  response.set_content(describeOf(segmentOf(request.matches[1])).JsonSerializable::serialize(), contentType.getName());
}

void EventBusController::publishOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!authorize(response, contentType, user, Access::Publish)) { return; }
  const std::optional<std::shared_ptr<IEventBus>> bus = busOf(response, contentType, segmentOf(request.matches[1]));
  if (!bus.has_value()) { return; }
  const auto badRequest = [&response, &contentType]() {
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
  };
  rapidjson::Document body;
  body.Parse(request.body.c_str());
  std::string topic;
  std::string type;
  if (body.HasParseError() || !json::getString(body, "TOPIC", topic) || !json::getString(body, "TYPE", type)
      || !isValidName(topic) || !isValidName(type)) {
    badRequest();
    return;
  }
  std::string payload;
  if (!optionalStringOf(body, "PAYLOAD", payload) || !isValidPayload(payload)) {
    badRequest();
    return;
  }
  std::string priorityName = "Normal";
  if (!optionalStringOf(body, "PRIORITY", priorityName)) {
    badRequest();
    return;
  }
  const std::optional<EventPriority> priority = priorityOf(priorityName);
  if (!priority.has_value()) {
    badRequest();
    return;
  }
  std::string targetName = "BUS";
  if (!optionalStringOf(body, "TARGET", targetName)) {
    badRequest();
    return;
  }
  const std::optional<bool> toSubscribers = toSubscribersOf(targetName);
  if (!toSubscribers.has_value()) {
    badRequest();
    return;
  }
  Event event(topic, type);
  event.setPriority(priority.value());
  event.setTimestamp(timestampNow());
  if (!payload.empty()) { event.setContent(reinterpret_cast<const std::byte *>(payload.data()), payload.size()); }
  const bool published = toSubscribers.value() ? (*bus)->publishToSubscribers(event) : (*bus)->publish(event);
  const std::int64_t sequence = (*bus)->sequenceOf();
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  writer.StartObject();
  writer.String("BUS");
  writer.String(segmentOf(request.matches[1]).c_str());
  writer.String("PRIORITY");
  writer.String(priorityNameOf(priority.value()));
  writer.String("PUBLISHED");
  writer.Bool(published);
  writer.String("SEQUENCE");
  writer.Int64(sequence);
  writer.String("TARGET");
  writer.String(toSubscribers.value() ? "SUBSCRIBERS" : "BUS");
  writer.String("TOPIC");
  writer.String(topic.c_str());
  writer.String("TYPE");
  writer.String(type.c_str());
  writer.EndObject();
  response.set_content(std::string(buffer.GetString()), contentType.getName());
  LOG_INFO(
    "published event '{}' ({}) on bus '{}': published={}", topic, type, segmentOf(request.matches[1]), published);
}

void EventBusController::subscribeOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!authorize(response, contentType, user, Access::Administer)) { return; }
  const std::optional<std::shared_ptr<IEventBus>> bus = busOf(response, contentType, segmentOf(request.matches[1]));
  if (!bus.has_value()) { return; }
  rapidjson::Document body;
  body.Parse(request.body.c_str());
  std::string topic;
  std::string subscriber;
  if (body.HasParseError() || !json::getString(body, "TOPIC", topic) || !json::getString(body, "SUBSCRIBER", subscriber)
      || !isValidName(topic) || !isValidName(subscriber)) {
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  // An exact topic has to be registered before a subscriber may join it, while
  // a wildcard pattern registers itself inside subscribe(). Registering here
  // keeps the endpoint usable without a separate topic route.
  if (!(*bus)->isTopicRegistered(topic) && !(*bus)->registerTopic(topic)) {
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  const bool subscribed = (*bus)->subscribe(topic, subscriber);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  writer.StartObject();
  writer.String("BUS");
  writer.String(segmentOf(request.matches[1]).c_str());
  writer.String("SUBSCRIBER");
  writer.String(subscriber.c_str());
  writer.String("SUBSCRIBED");
  writer.Bool(subscribed);
  writer.String("TOPIC");
  writer.String(topic.c_str());
  writer.EndObject();
  response.set_content(std::string(buffer.GetString()), contentType.getName());
  LOG_INFO("subscriber '{}' of bus '{}': topic='{}' subscribed={}",
    subscriber,
    segmentOf(request.matches[1]),
    topic,
    subscribed);
}

void EventBusController::unsubscribeOfDelete(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!authorize(response, contentType, user, Access::Administer)) { return; }
  const std::optional<std::shared_ptr<IEventBus>> bus = busOf(response, contentType, segmentOf(request.matches[1]));
  if (!bus.has_value()) { return; }
  rapidjson::Document body;
  body.Parse(request.body.c_str());
  std::string topic;
  std::string subscriber;
  if (body.HasParseError() || !json::getString(body, "TOPIC", topic) || !json::getString(body, "SUBSCRIBER", subscriber)
      || !isValidName(topic) || !isValidName(subscriber)) {
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  const bool unsubscribed = (*bus)->unsubscribe(topic, subscriber);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  writer.StartObject();
  writer.String("BUS");
  writer.String(segmentOf(request.matches[1]).c_str());
  writer.String("SUBSCRIBER");
  writer.String(subscriber.c_str());
  writer.String("TOPIC");
  writer.String(topic.c_str());
  writer.String("UNSUBSCRIBED");
  writer.Bool(unsubscribed);
  writer.EndObject();
  response.set_content(std::string(buffer.GetString()), contentType.getName());
  LOG_INFO("subscriber '{}' of bus '{}': topic='{}' unsubscribed={}",
    subscriber,
    segmentOf(request.matches[1]),
    topic,
    unsubscribed);
}

void EventBusController::depthOfGet(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!authorize(response, contentType, user, Access::Read)) { return; }
  if (!busOf(response, contentType, segmentOf(request.matches[1])).has_value()) { return; }
  const std::string subscriber = segmentOf(request.matches[2]);
  if (!isValidName(subscriber)) {
    response.status = HttpStatusCodes::BadRequest;
    response.set_content("", contentType.getName());
    return;
  }
  const std::size_t depth = m_eventBusService->subscriberQueueDepthOf(segmentOf(request.matches[1]), subscriber);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  writer.StartObject();
  writer.String("BUS");
  writer.String(segmentOf(request.matches[1]).c_str());
  writer.String("DEPTH");
  writer.Uint64(static_cast<std::uint64_t>(depth));
  writer.String("SUBSCRIBER");
  writer.String(subscriber.c_str());
  writer.EndObject();
  response.set_content(std::string(buffer.GetString()), contentType.getName());
}

void EventBusController::resetOfPost(const httplib::Request &request,
  httplib::Response &response,
  const ContentType &contentType,
  const std::optional<UserToken> &user)
{
  if (!authorize(response, contentType, user, Access::Administer)) { return; }
  const std::optional<std::shared_ptr<IEventBus>> bus = busOf(response, contentType, segmentOf(request.matches[1]));
  if (!bus.has_value()) { return; }
  (*bus)->reset();
  rapidjson::StringBuffer buffer;
  rapidjson::Writer writer(buffer);
  writer.StartObject();
  writer.String("BUS");
  writer.String(segmentOf(request.matches[1]).c_str());
  writer.String("RESET");
  writer.Bool(true);
  writer.EndObject();
  response.set_content(std::string(buffer.GetString()), contentType.getName());
  LOG_INFO("reset event bus '{}'", segmentOf(request.matches[1]));
}
}// namespace lintel
