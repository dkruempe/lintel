#include "lintel/features/base/models/MessageQueueStats.h"

#include <algorithm>

#include <boost/interprocess/creation_tags.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "lintel/features/base/configuration/MessageQueueEntry.h"
#include "lintel/features/base/msg/Message.h"
#include "lintel/features/base/msg/MessageQueueCore.h"

namespace lintel {
namespace {
  /** Suffix of the dead-letter queue that belongs to a queue (see MQ-3). */
  constexpr const char *kDeadLetterSuffix = ".dlq";

  /** @return milliseconds since the epoch for @p timePoint */
  std::int64_t epochMillisecondsOf(const std::chrono::system_clock::time_point &timePoint)
  {
    return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
  }

  /** @return true if a posix message queue with the given name exists */
  bool messageQueueExistsOf(const std::string &queueName)
  {
    try {
      boost::interprocess::message_queue queue(boost::interprocess::open_only, queueName.c_str());
      return true;
    } catch (const boost::interprocess::interprocess_exception &) {
      return false;
    }
  }

  /** @return number of messages in the dead-letter queue, nullopt if none exists */
  std::optional<std::int32_t> deadLetterQueueSizeOf(const std::string &deadLetterQueueName)
  {
    if (!messageQueueExistsOf(deadLetterQueueName)) { return std::nullopt; }
    return MessageQueueCore::numberMessagesOfIfExistsOf(deadLetterQueueName);
  }
}// namespace

MessageQueueStats messageQueueStatsOf(const MessageQueueEntry &entry,
  std::int32_t depth,
  const std::chrono::system_clock::time_point &now,
  const std::chrono::seconds &staleThreshold)
{
  MessageQueueStats stats;
  stats.depth = depth;
  stats.maxMessages = entry.get_max_messages();
  stats.consumerLag = depth;
  stats.ownerPid = entry.get_owner_pid();
  stats.ownerHost = entry.get_owner_host();
  stats.ownerState = entry.get_owner_state();
  const std::optional<std::chrono::system_clock::time_point> startedAt = entry.get_started_at();
  if (startedAt.has_value()) { stats.startedAtEpochMs = epochMillisecondsOf(startedAt.value()); }
  const std::optional<std::chrono::system_clock::time_point> lastHeartbeatAt = entry.get_last_heartbeat_at();
  if (lastHeartbeatAt.has_value()) {
    const auto heartbeat = lastHeartbeatAt.value();
    stats.lastHeartbeatAtEpochMs = epochMillisecondsOf(heartbeat);
    const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - heartbeat);
    stats.heartbeatAgeMs = std::max<std::int64_t>(age.count(), 0);
  }
  const bool activeState = !stats.ownerState.has_value() || stats.ownerState.value() == "active";
  const bool heartbeatFresh =
    stats.heartbeatAgeMs.has_value() && stats.heartbeatAgeMs.value() <= staleThreshold.count() * 1000;
  stats.alive = activeState && heartbeatFresh;

  const std::string deadLetterQueueName = entry.get_message_queue_name() + kDeadLetterSuffix;
  stats.deadLetterQueueSize = deadLetterQueueSizeOf(deadLetterQueueName);
  if (stats.deadLetterQueueSize.has_value()) { stats.deadLetterQueueName = deadLetterQueueName; }
  return stats;
}

std::int32_t purgeMessageQueueOf(const std::string &queueName, std::int32_t maxMessages)
{
  // never create the queue just to purge it: an idle queue that no producer has
  // opened yet simply holds no messages
  if (maxMessages <= 0 || !messageQueueExistsOf(queueName)) { return 0; }
  // the queue is opened with the size of Message: that is the message size all
  // MessageQueue<Message> instances of this project use.
  MessageQueueCore messageQueue(queueName, maxMessages, sizeof(Message));
  Message message;
  std::int32_t purged = 0;
  while (messageQueue.tryReceiveOf(&message, sizeof(Message))) { ++purged; }
  return purged;
}
}// namespace lintel
