#include "lintel/features/base/controller/EventBusDtos.h"

#include "lintel/core/exceptions/HttpBadRequestException.h"
#include "lintel/core/utils/JsonUtils.h"

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <optional>
#include <utility>

namespace lintel {
namespace {
  /** @param policy the drop policy
   * @return the name used on the wire */
  const char *dropPolicyNameOf(DropPolicy policy)
  {
    switch (policy) {
    case DropPolicy::Fail:
      return "Fail";
    case DropPolicy::DropOldest:
      return "DropOldest";
    case DropPolicy::DropNewest:
      return "DropNewest";
    case DropPolicy::Block:
      return "Block";
    }
    return "Fail";
  }

  /** @param name the name used on the wire
   * @return the drop policy, empty for an unknown name */
  std::optional<DropPolicy> dropPolicyOf(const std::string &name)
  {
    if (name == "Fail") { return DropPolicy::Fail; }
    if (name == "DropOldest") { return DropPolicy::DropOldest; }
    if (name == "DropNewest") { return DropPolicy::DropNewest; }
    if (name == "Block") { return DropPolicy::Block; }
    return std::nullopt;
  }
}// namespace

EventBusDto::EventBusDto(std::string name, const EventBusConfig &config) : m_name(std::move(name)), m_config(config) {}

EventBusDto::EventBusDto(std::string name) : m_name(std::move(name)) {}

void EventBusDto::markAvailable(const EventBusCounters &counters)
{
  m_available = true;
  m_dropped = counters.dropped;
  m_topics = counters.topics;
  m_subscribers = counters.subscribers;
  m_busDepth = counters.busDepth;
  m_sequence = counters.sequence;
}

const std::string &EventBusDto::getName() const { return m_name; }

const std::optional<EventBusConfig> &EventBusDto::getConfig() const { return m_config; }

bool EventBusDto::isAvailable() const { return m_available; }

std::uint64_t EventBusDto::getDropped() const { return m_dropped; }

std::size_t EventBusDto::getTopics() const { return m_topics; }

std::size_t EventBusDto::getSubscribers() const { return m_subscribers; }

std::size_t EventBusDto::getBusDepth() const { return m_busDepth; }

std::int64_t EventBusDto::getSequence() const { return m_sequence; }

void EventBusDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  writer->String("NAME");
  writer->String(m_name.c_str());
  writer->String("AVAILABLE");
  writer->Bool(m_available);
  // the configuration is omitted for a bus whose bootstrap entry is invalid
  if (m_config.has_value()) {
    const EventBusConfig &config = m_config.value();
    writer->String("BUS_CAPACITY");
    writer->Uint64(static_cast<std::uint64_t>(config.busCapacity));
    writer->String("SUBSCRIBER_CAPACITY");
    writer->Uint64(static_cast<std::uint64_t>(config.subscriberCapacity));
    writer->String("MAX_TOPICS");
    writer->Uint64(static_cast<std::uint64_t>(config.maxTopics));
    writer->String("MAX_SUBSCRIBERS");
    writer->Uint64(static_cast<std::uint64_t>(config.maxSubscribers));
    writer->String("MAX_PRIORITIES");
    writer->Uint64(static_cast<std::uint64_t>(config.maxPriorities));
    writer->String("PUB_POLICY");
    writer->String(dropPolicyNameOf(config.pubPolicy));
    writer->String("STICKY");
    writer->Bool(config.sticky);
    writer->String("STICKY_TTL_MS");
    writer->Uint(config.stickyTtlMs);
  }
  // the runtime counters are only known while the bus can be opened
  if (m_available) {
    writer->String("DROPPED");
    writer->Uint64(m_dropped);
    writer->String("TOPIC_COUNT");
    writer->Uint64(static_cast<std::uint64_t>(m_topics));
    writer->String("SUBSCRIBER_COUNT");
    writer->Uint64(static_cast<std::uint64_t>(m_subscribers));
    writer->String("BUS_DEPTH");
    writer->Uint64(static_cast<std::uint64_t>(m_busDepth));
    writer->String("SEQUENCE");
    writer->Int64(m_sequence);
  }
  writer->EndObject();
}

bool EventBusDto::deserialize(const rapidjson::Value &obj)
{
  if (!obj.IsObject()) { return false; }
  std::string name;
  if (!json::getString(obj, "NAME", name)) { return false; }
  m_name = std::move(name);
  json::getBool(obj, "AVAILABLE", m_available);
  EventBusConfig config;
  bool hasConfig = false;
  std::uint64_t number = 0;
  if (json::getUint64(obj, "BUS_CAPACITY", number)) {
    config.busCapacity = static_cast<std::size_t>(number);
    hasConfig = true;
  }
  if (json::getUint64(obj, "SUBSCRIBER_CAPACITY", number)) {
    config.subscriberCapacity = static_cast<std::size_t>(number);
    hasConfig = true;
  }
  if (json::getUint64(obj, "MAX_TOPICS", number)) {
    config.maxTopics = static_cast<std::size_t>(number);
    hasConfig = true;
  }
  if (json::getUint64(obj, "MAX_SUBSCRIBERS", number)) {
    config.maxSubscribers = static_cast<std::size_t>(number);
    hasConfig = true;
  }
  if (json::getUint64(obj, "MAX_PRIORITIES", number)) {
    config.maxPriorities = static_cast<std::size_t>(number);
    hasConfig = true;
  }
  std::string policyName;
  if (json::getString(obj, "PUB_POLICY", policyName)) {
    const std::optional<DropPolicy> policy = dropPolicyOf(policyName);
    if (!policy.has_value()) { return false; }
    config.pubPolicy = policy.value();
    hasConfig = true;
  }
  if (json::getBool(obj, "STICKY", config.sticky)) { hasConfig = true; }
  if (json::getUint64(obj, "STICKY_TTL_MS", number)) {
    config.stickyTtlMs = static_cast<std::uint32_t>(number);
    hasConfig = true;
  }
  m_config = hasConfig ? std::optional<EventBusConfig>(config) : std::nullopt;
  json::getUint64(obj, "DROPPED", m_dropped);
  if (json::getUint64(obj, "TOPIC_COUNT", number)) { m_topics = static_cast<std::size_t>(number); }
  if (json::getUint64(obj, "SUBSCRIBER_COUNT", number)) { m_subscribers = static_cast<std::size_t>(number); }
  if (json::getUint64(obj, "BUS_DEPTH", number)) { m_busDepth = static_cast<std::size_t>(number); }
  json::getInt64(obj, "SEQUENCE", m_sequence);
  return true;
}

EventBusesDto::EventBusesDto(std::vector<EventBusDto> eventBusDtos) : m_eventBusDtos(std::move(eventBusDtos)) {}

const std::vector<EventBusDto> &EventBusesDto::getEventBusDtos() const { return m_eventBusDtos; }

void EventBusesDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartArray();
  for (const auto &eventBusDto : m_eventBusDtos) { eventBusDto.serialize(writer); }
  writer->EndArray();
}

void EventBusesDto::deserialize(const std::string &json)
{
  rapidjson::Document document;
  document.Parse(json.c_str());
  if (!document.IsArray()) { throw HttpBadRequestException(); }
  m_eventBusDtos.clear();
  for (const auto &entry : document.GetArray()) {
    if (!entry.IsObject()) { continue; }
    EventBusDto eventBusDto;
    if (eventBusDto.deserialize(entry)) { m_eventBusDtos.push_back(std::move(eventBusDto)); }
  }
}
}// namespace lintel
