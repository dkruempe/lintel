#ifndef LINTEL_MESSAGEQUEUEQUERY_H
#define LINTEL_MESSAGEQUEUEQUERY_H

#include <chrono>
#include <cstdint>
#include <string>

namespace lintel {
/**
 * Regex filters of a message queue query. Both fields are patterns (the
 * persistence layer uses REGEXP / ~*), so naming them on every call site is
 * easy to get wrong - a named struct makes the two positions unambiguous.
 */
struct MessageQueueFilter
{
  /** Process name pattern; pass ".*" to match every process */
  std::string processName = ".*";
  /** Queue name pattern; pass ".*" to match every queue */
  std::string queueName = ".*";

  /** Equality so the filter can be matched by value (trompeloeil, tests). */
  friend bool operator==(const MessageQueueFilter &lhs, const MessageQueueFilter &rhs)
  {
    return lhs.processName == rhs.processName && lhs.queueName == rhs.queueName;
  }
};

/**
 * Everything needed to register the owner of a message queue. The three names
 * (queue, process, host) and the two timestamps are same-typed, so they live in
 * one named aggregate instead of six positional constructor arguments.
 */
struct MessageQueueOwnerRegistration
{
  /** Name of the queue being claimed */
  std::string queueName;
  /** Name of the claiming process */
  std::string processName;
  /** Host the owning process runs on */
  std::string hostName;
  /** PID of the owning process */
  std::uint64_t pid = 0;
  /** When the owner registered the queue */
  std::chrono::system_clock::time_point startedAt;
  /** Liveness state reported by the owner */
  std::string state = "active";
};

/**
 * A liveness-state update for an existing owner: which queue and the new state.
 * A named aggregate keeps the two same-typed strings from being swapped.
 */
struct MessageQueueOwnerStateUpdate
{
  /** Name of the queue whose owner state changes */
  std::string queueName;
  /** The new owner state (e.g. "active") */
  std::string state;
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEQUERY_H
