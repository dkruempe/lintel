#include "lintel/features/base/controller/MessageQueueDto.h"

#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/models/MessageQueueStats.h"

#include "lintel/core/services/LoggerService.h"
#include "lintel/core/utils/JsonUtils.h"

#include <rapidjson/stringbuffer.h>

namespace lintel {
MessageQueueDto::Shapes MessageQueueDto::m_shape{};


MessageQueueDto::MessageQueueDto(const MessageQueueEntry &messsageQueueEntry, int32_t messages)
  : MessageQueueDto(messsageQueueEntry, messageQueueStatsOf(messsageQueueEntry, messages))
{}

MessageQueueDto::MessageQueueDto(const MessageQueueEntry &messsageQueueEntry, const MessageQueueStats &stats)
  : m_name(messsageQueueEntry.get_message_queue_name()), m_process(messsageQueueEntry.get_process_name()),
    m_maxMessages(stats.maxMessages), m_messages(stats.depth), m_consumerLag(stats.consumerLag),
    m_ownerPid(stats.ownerPid), m_ownerHost(stats.ownerHost), m_ownerState(stats.ownerState),
    m_startedAtEpochMs(stats.startedAtEpochMs), m_lastHeartbeatAtEpochMs(stats.lastHeartbeatAtEpochMs),
    m_heartbeatAgeMs(stats.heartbeatAgeMs), m_alive(stats.alive), m_deadLetterQueueName(stats.deadLetterQueueName),
    m_deadLetterQueueSize(stats.deadLetterQueueSize)
{}

const std::string &MessageQueueDto::getName() const { return m_name; }

const std::string &MessageQueueDto::getProcess() const { return m_process; }

int32_t MessageQueueDto::getMaxMessages() const { return m_maxMessages; }

int32_t MessageQueueDto::getMessages() const { return m_messages; }

int64_t MessageQueueDto::getConsumerLag() const { return m_consumerLag; }

const std::optional<std::uint64_t> &MessageQueueDto::getOwnerPid() const { return m_ownerPid; }

const std::optional<std::string> &MessageQueueDto::getOwnerHost() const { return m_ownerHost; }

const std::optional<std::string> &MessageQueueDto::getOwnerState() const { return m_ownerState; }

const std::optional<std::int64_t> &MessageQueueDto::getStartedAtEpochMs() const { return m_startedAtEpochMs; }

const std::optional<std::int64_t> &MessageQueueDto::getLastHeartbeatAtEpochMs() const
{
  return m_lastHeartbeatAtEpochMs;
}

const std::optional<std::int64_t> &MessageQueueDto::getHeartbeatAgeMs() const { return m_heartbeatAgeMs; }

bool MessageQueueDto::isAlive() const { return m_alive; }

const std::optional<std::string> &MessageQueueDto::getDeadLetterQueueName() const { return m_deadLetterQueueName; }

const std::optional<int32_t> &MessageQueueDto::getDeadLetterQueueSize() const { return m_deadLetterQueueSize; }

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
  // CONSUMER_LAG
  writer->String(m_shape.CONSUMER_LAG);
  writer->Int64(m_consumerLag);
  // ALIVE
  writer->String(m_shape.ALIVE);
  writer->Bool(m_alive);
  // OWNER_PID
  if (m_ownerPid.has_value()) {
    writer->String(m_shape.OWNER_PID);
    writer->Uint64(m_ownerPid.value());
  }
  // OWNER_HOST
  if (m_ownerHost.has_value()) {
    writer->String(m_shape.OWNER_HOST);
    writer->String(m_ownerHost.value().c_str());
  }
  // OWNER_STATE
  if (m_ownerState.has_value()) {
    writer->String(m_shape.OWNER_STATE);
    writer->String(m_ownerState.value().c_str());
  }
  // STARTED_AT
  if (m_startedAtEpochMs.has_value()) {
    writer->String(m_shape.STARTED_AT);
    writer->Int64(m_startedAtEpochMs.value());
  }
  // LAST_HEARTBEAT_AT
  if (m_lastHeartbeatAtEpochMs.has_value()) {
    writer->String(m_shape.LAST_HEARTBEAT_AT);
    writer->Int64(m_lastHeartbeatAtEpochMs.value());
  }
  // HEARTBEAT_AGE_MS
  if (m_heartbeatAgeMs.has_value()) {
    writer->String(m_shape.HEARTBEAT_AGE_MS);
    writer->Int64(m_heartbeatAgeMs.value());
  }
  // DEAD_LETTER_QUEUE
  if (m_deadLetterQueueName.has_value()) {
    writer->String(m_shape.DEAD_LETTER_QUEUE);
    writer->String(m_deadLetterQueueName.value().c_str());
  }
  // DEAD_LETTER_SIZE
  if (m_deadLetterQueueSize.has_value()) {
    writer->String(m_shape.DEAD_LETTER_SIZE);
    writer->Int(m_deadLetterQueueSize.value());
  }
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
  // the statistics fields are optional so that older payloads stay readable
  json::getInt64(obj, m_shape.CONSUMER_LAG, m_consumerLag);
  json::getBool(obj, m_shape.ALIVE, m_alive);
  std::uint64_t ownerPid = 0;
  if (json::getUint64(obj, m_shape.OWNER_PID, ownerPid)) { m_ownerPid = ownerPid; }
  std::string ownerHost;
  if (json::getString(obj, m_shape.OWNER_HOST, ownerHost)) { m_ownerHost = ownerHost; }
  std::string ownerState;
  if (json::getString(obj, m_shape.OWNER_STATE, ownerState)) { m_ownerState = ownerState; }
  std::int64_t startedAt = 0;
  if (json::getInt64(obj, m_shape.STARTED_AT, startedAt)) { m_startedAtEpochMs = startedAt; }
  std::int64_t lastHeartbeatAt = 0;
  if (json::getInt64(obj, m_shape.LAST_HEARTBEAT_AT, lastHeartbeatAt)) { m_lastHeartbeatAtEpochMs = lastHeartbeatAt; }
  std::int64_t heartbeatAge = 0;
  if (json::getInt64(obj, m_shape.HEARTBEAT_AGE_MS, heartbeatAge)) { m_heartbeatAgeMs = heartbeatAge; }
  std::string deadLetterQueue;
  if (json::getString(obj, m_shape.DEAD_LETTER_QUEUE, deadLetterQueue)) { m_deadLetterQueueName = deadLetterQueue; }
  std::int32_t deadLetterSize = 0;
  if (json::getInt(obj, m_shape.DEAD_LETTER_SIZE, deadLetterSize)) { m_deadLetterQueueSize = deadLetterSize; }
  return success;
}
}// namespace lintel
