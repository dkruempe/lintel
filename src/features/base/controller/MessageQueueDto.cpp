#include "lintel/features/base/controller/MessageQueueDto.h"

#include "lintel/features/base/configuration/MessageQueueEntry.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

#include <rapidjson/stringbuffer.h>

namespace lintel {
MessageQueueDto::Shapes MessageQueueDto::m_shape{};


MessageQueueDto::MessageQueueDto(const MessageQueueEntry &messsageQueueEntry, int32_t messages)
  : m_name(messsageQueueEntry.get_message_queue_name()), m_process(messsageQueueEntry.get_process_name()),
    m_maxMessages(messsageQueueEntry.get_max_messages()), m_messages(messages)
{}

const std::string &MessageQueueDto::getName() const { return m_name; }

const std::string &MessageQueueDto::getProcess() const { return m_process; }

int32_t MessageQueueDto::getMaxMessages() const { return m_maxMessages; }

int32_t MessageQueueDto::getMessages() const { return m_messages; }

void MessageQueueDto::serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const
{
  writer->StartObject();
  // NAME
  writer->String(m_shape.NAME);
  writer->String(m_name.c_str());
  // PROCESS_NAME
  writer->String(m_shape.PROCESS);
  writer->String(m_process.c_str());
  // MAX_MESSAGES
  writer->String(m_shape.MAX_MESSAGES);
  writer->Int(m_maxMessages);
  // MESSAGES
  writer->String(m_shape.MESSAGES);
  writer->Int(m_messages);
  writer->EndObject();
}

bool MessageQueueDto::deserialize(const rapidjson::Value &obj)
{
  bool success = true;
  if (!json::getString(obj, m_shape.NAME, m_name)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.NAME);
  }
  if (!json::getString(obj, m_shape.PROCESS, m_process)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.PROCESS);
  }
  if (!json::getInt(obj, m_shape.MAX_MESSAGES, m_maxMessages)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.MAX_MESSAGES);
  }
  if (!json::getInt(obj, m_shape.MESSAGES, m_messages)) {
    success = false;
    LOG_ERROR("{} not defined in json serialization", m_shape.MESSAGES);
  }
  return success;
}
}// namespace lintel
