#ifndef LINTEL_MESSAGEQUEUEDTO_H
#define LINTEL_MESSAGEQUEUEDTO_H

#include "lintel/core/models/JsonSerializable.h"
#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include <cstdint>
#include <optional>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <string>

namespace lintel {
struct MessageQueueStats;

/** DTO representing a single message queue with its current state */
class MessageQueueDto : public JsonSerializable
{
  /** The queue name */
  std::string m_name;
  /** The owning process name */
  std::string m_process;
  /** The maximum number of messages */
  int32_t m_maxMessages{};
  /** The current number of messages */
  int32_t m_messages{};
  /** Messages still waiting for a consumer */
  int64_t m_consumerLag{};
  /** Persisted owner pid */
  std::optional<std::uint64_t> m_ownerPid;
  /** Persisted owner host */
  std::optional<std::string> m_ownerHost;
  /** Persisted owner state */
  std::optional<std::string> m_ownerState;
  /** Owner start time as milliseconds since the epoch */
  std::optional<std::int64_t> m_startedAtEpochMs;
  /** Last heartbeat as milliseconds since the epoch */
  std::optional<std::int64_t> m_lastHeartbeatAtEpochMs;
  /** Age of the last heartbeat in milliseconds */
  std::optional<std::int64_t> m_heartbeatAgeMs;
  /** True if the owner heartbeat is fresh */
  bool m_alive = false;
  /** Name of the dead-letter queue, if one exists */
  std::optional<std::string> m_deadLetterQueueName;
  /** Number of messages in the dead-letter queue, if one exists */
  std::optional<int32_t> m_deadLetterQueueSize;

  /** JSON field name constants */
  static struct Shapes
  {
    const char *const NAME = "NAME";
    const char *const PROCESS = "PROCESS";
    const char *const MAX_MESSAGES = "MAX_MESSAGES";
    const char *const MESSAGES = "MESSAGES";
    const char *const CONSUMER_LAG = "CONSUMER_LAG";
    const char *const OWNER_PID = "OWNER_PID";
    const char *const OWNER_HOST = "OWNER_HOST";
    const char *const OWNER_STATE = "OWNER_STATE";
    const char *const STARTED_AT = "STARTED_AT";
    const char *const LAST_HEARTBEAT_AT = "LAST_HEARTBEAT_AT";
    const char *const HEARTBEAT_AGE_MS = "HEARTBEAT_AGE_MS";
    const char *const ALIVE = "ALIVE";
    const char *const DEAD_LETTER_QUEUE = "DEAD_LETTER_QUEUE";
    const char *const DEAD_LETTER_SIZE = "DEAD_LETTER_SIZE";
  } m_shape;

public:
  /** Default constructor */
  MessageQueueDto() = default;

  /** Construct from a MessageQueueEntry and current message count
   * @param messsageQueueEntry The configuration entry
   * @param messages The current message count */
  MessageQueueDto(const MessageQueueEntry &messsageQueueEntry, int32_t messages);

  /** Construct from a MessageQueueEntry and its gathered statistics
   * @param messsageQueueEntry The configuration entry
   * @param stats The gathered statistics */
  MessageQueueDto(const MessageQueueEntry &messsageQueueEntry, const MessageQueueStats &stats);

  /** Get the queue name
   * @return The name */
  [[nodiscard]] const std::string &getName() const;

  /** Get the owning process name
   * @return The process name */
  [[nodiscard]] const std::string &getProcess() const;

  /** Get the maximum message count
   * @return The max messages */
  [[nodiscard]] int32_t getMaxMessages() const;

  /** Get the current message count
   * @return The current messages */
  [[nodiscard]] int32_t getMessages() const;

  /** Get the number of messages still waiting for a consumer
   * @return The consumer lag */
  [[nodiscard]] int64_t getConsumerLag() const;

  /** Get the persisted owner pid
   * @return The owner pid, if one was registered */
  [[nodiscard]] const std::optional<std::uint64_t> &getOwnerPid() const;

  /** Get the persisted owner host
   * @return The owner host, if one was registered */
  [[nodiscard]] const std::optional<std::string> &getOwnerHost() const;

  /** Get the persisted owner state
   * @return The owner state, if one was registered */
  [[nodiscard]] const std::optional<std::string> &getOwnerState() const;

  /** Get the owner start time
   * @return Milliseconds since the epoch, if present */
  [[nodiscard]] const std::optional<std::int64_t> &getStartedAtEpochMs() const;

  /** Get the last heartbeat time
   * @return Milliseconds since the epoch, if present */
  [[nodiscard]] const std::optional<std::int64_t> &getLastHeartbeatAtEpochMs() const;

  /** Get the age of the last heartbeat
   * @return Age in milliseconds, if present */
  [[nodiscard]] const std::optional<std::int64_t> &getHeartbeatAgeMs() const;

  /** @return true if the owner heartbeat is fresh */
  [[nodiscard]] bool isAlive() const;

  /** Get the dead-letter queue name
   * @return The name, if a dead-letter queue exists */
  [[nodiscard]] const std::optional<std::string> &getDeadLetterQueueName() const;

  /** Get the dead-letter queue depth
   * @return The message count, if a dead-letter queue exists */
  [[nodiscard]] const std::optional<int32_t> &getDeadLetterQueueSize() const;

  /** Serialize to JSON
   * @param writer The rapidjson writer */
  void serialize(rapidjson::Writer<rapidjson::StringBuffer> *writer) const override;

  /** Deserialize from JSON
   * @param obj The JSON value
   * @return True on success */
  bool deserialize(const rapidjson::Value &obj) override;
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEDTO_H
