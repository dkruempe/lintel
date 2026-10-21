#include "lintel/features/base/controller/MessageQueueDtos.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

namespace lintel {
MessageQueueDtos::MessageQueueDtos(const std::vector<std::pair<MessageQueueEntry, int32_t>> &messageQueueDtos)
  : m_messageQueueDtos(build(messageQueueDtos))
{}

std::vector<MessageQueueDto> MessageQueueDtos::build(const std::vector<std::pair<MessageQueueEntry, int32_t>> &pairs)
{
  std::vector<MessageQueueDto> messageQueueDtos;
  messageQueueDtos.reserve(pairs.size());
  for (const auto &[entry, messages] : pairs) { messageQueueDtos.emplace_back(entry, messages); }
  return messageQueueDtos;
}

const std::vector<MessageQueueDto> &MessageQueueDtos::getMessageQueueDtos() const { return m_messageQueueDtos; }

void MessageQueueDtos::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartArray();
  for (const auto &messageQueueDto : m_messageQueueDtos) { messageQueueDto.serialize(writer); }
  writer->EndArray();
}

void MessageQueueDtos::deserialize(const std::string &json)
{
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  if (!doc.IsArray()) { return; }
  for (const auto &messageQueue : doc.GetArray()) {
    if (!messageQueue.IsObject()) { continue; }
    MessageQueueDto messageQueueDto;
    messageQueueDto.deserialize(messageQueue);
    m_messageQueueDtos.push_back(messageQueueDto);
  }
}

MessageQueuesPageDto::Shapes MessageQueuesPageDto::shape{};

MessageQueuesPageDto::MessageQueuesPageDto(const Page<MessageQueueEntry> &page)
{
  m_messageQueueDtos.reserve(page.getItems().size());
  for (const auto &entry : page.getItems()) { m_messageQueueDtos.emplace_back(entry, 0); }
  m_hasMore = page.hasMore();
  m_nextAfter = page.getNextAfter();
}

const std::vector<MessageQueueDto> &MessageQueuesPageDto::getMessageQueueDtos() const { return m_messageQueueDtos; }

bool MessageQueuesPageDto::hasMore() const { return m_hasMore; }

const std::optional<std::string> &MessageQueuesPageDto::getNextAfter() const { return m_nextAfter; }

void MessageQueuesPageDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  writer->String(shape.ITEMS);
  writer->StartArray();
  for (const auto &messageQueueDto : m_messageQueueDtos) { messageQueueDto.serialize(writer); }
  writer->EndArray();
  writer->String(shape.HAS_MORE);
  writer->Bool(m_hasMore);
  if (m_nextAfter.has_value()) {
    writer->String(shape.NEXT_AFTER);
    writer->String(m_nextAfter.value().c_str());
  }
  writer->EndObject();
}

void MessageQueuesPageDto::deserialize(const std::string &json)
{
  rapidjson::Document doc;
  doc.Parse(json.c_str());
  if (!doc.IsObject()) { return; }
  if (doc.HasMember(shape.ITEMS) && doc[shape.ITEMS].IsArray()) {
    for (const auto &messageQueue : doc[shape.ITEMS].GetArray()) {
      if (!messageQueue.IsObject()) { continue; }
      MessageQueueDto messageQueueDto;
      messageQueueDto.deserialize(messageQueue);
      m_messageQueueDtos.push_back(messageQueueDto);
    }
  }
  if (doc.HasMember(shape.HAS_MORE) && doc[shape.HAS_MORE].IsBool()) { m_hasMore = doc[shape.HAS_MORE].GetBool(); }
  if (doc.HasMember(shape.NEXT_AFTER) && doc[shape.NEXT_AFTER].IsString()) {
    m_nextAfter = std::make_optional(doc[shape.NEXT_AFTER].GetString());
  }
}

MessageQueuePurgeDto::Shapes MessageQueuePurgeDto::shape{};

MessageQueuePurgeDto::MessageQueuePurgeDto(std::string name, int32_t purged) : m_name(std::move(name)), m_purged(purged)
{}

const std::string &MessageQueuePurgeDto::getName() const { return m_name; }

int32_t MessageQueuePurgeDto::getPurged() const { return m_purged; }

void MessageQueuePurgeDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  writer->String(shape.NAME);
  writer->String(m_name.c_str());
  writer->String(shape.PURGED);
  writer->Int(m_purged);
  writer->EndObject();
}

bool MessageQueuePurgeDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  if (!json::getString(obj, shape.NAME, m_name)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.NAME);
  }
  if (!json::getInt(obj, shape.PURGED, m_purged)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", shape.PURGED);
  }
  return success;
}
}// namespace lintel
