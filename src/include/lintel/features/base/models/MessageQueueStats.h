#ifndef LINTEL_MESSAGEQUEUESTATS_H
#define LINTEL_MESSAGEQUEUESTATS_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace lintel {
class MessageQueueEntry;

/**
 * Read-only operational state of a single message queue.
 *
 * Depth is the number of messages currently queued. There is no consumer
 * cursor in the queue itself, so the consumer lag is the number of messages
 * that still wait for a consumer, i.e. the depth.
 */
struct MessageQueueStats
{
  /** Number of messages currently in the queue */
  std::int32_t depth = 0;
  /** Configured maximum number of messages */
  std::int32_t maxMessages = 0;
  /** Messages still waiting for a consumer (equals depth) */
  std::int64_t consumerLag = 0;
  /** Persisted owner pid, if an owner registered the queue */
  std::optional<std::uint64_t> ownerPid;
  /** Persisted owner host, if an owner registered the queue */
  std::optional<std::string> ownerHost;
  /** Persisted owner state (e.g. "active") */
  std::optional<std::string> ownerState;
  /** Owner start time as milliseconds since the epoch */
  std::optional<std::int64_t> startedAtEpochMs;
  /** Last heartbeat as milliseconds since the epoch */
  std::optional<std::int64_t> lastHeartbeatAtEpochMs;
  /** Age of the last heartbeat in milliseconds */
  std::optional<std::int64_t> heartbeatAgeMs;
  /** True if an owner is registered and its heartbeat is fresh */
  bool alive = false;
  /** Name of the configured dead-letter queue, if one exists */
  std::optional<std::string> deadLetterQueueName;
  /** Number of messages in the dead-letter queue, if one exists */
  std::optional<std::int32_t> deadLetterQueueSize;
};

/**
 * Collect the operational state of a queue.
 * @param entry queue entry carrying the owner metadata
 * @param depth current number of messages in the queue
 * @param now reference time for the heartbeat age
 * @param staleThreshold heartbeat age above which the owner counts as stale
 * @return statistics of the queue
 */
MessageQueueStats messageQueueStatsOf(const MessageQueueEntry &entry,
  std::int32_t depth,
  const std::chrono::system_clock::time_point &now = std::chrono::system_clock::now(),
  const std::chrono::seconds &staleThreshold = std::chrono::seconds{ 60 });

/**
 * Remove all messages that are currently in the queue. This is destructive and
 * therefore meant for explicit operator actions only.
 *
 * @param queueName name of the queue to drain
 * @param maxMessages configured capacity of the queue, used to open it
 * @return number of messages that were removed
 */
std::int32_t purgeMessageQueueOf(const std::string &queueName, std::int32_t maxMessages);

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUESTATS_H
