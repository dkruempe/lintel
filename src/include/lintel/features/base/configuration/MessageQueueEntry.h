#ifndef LINTEL_MESSAGEQUEUEENTRY_H
#define LINTEL_MESSAGEQUEUEENTRY_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "lintel/core/configuration/Entry.h"

namespace lintel {
/** The owner columns of a message queue row as they are stored: all optional,
 *  because a row written before the owner schema existed has none of them, and
 *  because clearing the owner writes NULLs back. Grouping them also keeps the
 *  two same-typed timestamps from being swapped at the call site. */
struct MessageQueueOwnerColumns
{
  /** PID of the owning process */
  std::optional<std::uint64_t> pid;
  /** Host the owner runs on */
  std::optional<std::string> host;
  /** When the owner registered the queue */
  std::optional<std::chrono::system_clock::time_point> startedAt;
  /** When the owner last checked in */
  std::optional<std::chrono::system_clock::time_point> lastHeartbeatAt;
  /** Liveness state reported by the owner */
  std::optional<std::string> state;
};

/** Configuration entry for a message queue */
class MessageQueueEntry : public Entry
{
private:
  /** The process name that owns the queue */
  const std::string processName;
  /** The name of the message queue */
  const std::string messageQueueName;
  /** The maximum number of messages the queue can hold */
  const int32_t maxMessages;
  std::optional<std::uint64_t> m_ownerPid;
  std::optional<std::string> m_ownerHost;
  std::optional<std::chrono::system_clock::time_point> m_startedAt;
  std::optional<std::chrono::system_clock::time_point> m_lastHeartbeatAt;
  std::optional<std::string> m_ownerState;

public:
  /** Construct a message queue entry
   * @param component The configuration component name
   * @param processName The owning process name
   * @param messageQueueName The queue name
   * @param maxMessages The maximum message count */
  MessageQueueEntry(const std::string_view &component,
    std::string processName,
    std::string messageQueueName,
    int32_t maxMessages);

  /** Construct a message queue entry including its owner record
   * @param component The configuration component name
   * @param processName The owning process name
   * @param messageQueueName The queue name
   * @param maxMessages The maximum message count
   * @param owner Who owns the queue and since when it is alive */
  MessageQueueEntry(const std::string_view &component,
    std::string processName,
    std::string messageQueueName,
    int32_t maxMessages,
    MessageQueueOwnerColumns owner);

  /** Get the process name
   * @return The process name */
  [[nodiscard]] const std::string &get_process_name() const;

  /** Get the message queue name
   * @return The queue name */
  [[nodiscard]] const std::string &get_message_queue_name() const;

  /** Get the maximum message count
   * @return The max messages */
  [[nodiscard]] constexpr int32_t get_max_messages() const { return maxMessages; }

  [[nodiscard]] const std::optional<std::uint64_t> &get_owner_pid() const { return m_ownerPid; }
  [[nodiscard]] const std::optional<std::string> &get_owner_host() const { return m_ownerHost; }
  [[nodiscard]] const std::optional<std::chrono::system_clock::time_point> &get_started_at() const
  {
    return m_startedAt;
  }
  [[nodiscard]] const std::optional<std::chrono::system_clock::time_point> &get_last_heartbeat_at() const
  {
    return m_lastHeartbeatAt;
  }
  [[nodiscard]] const std::optional<std::string> &get_owner_state() const { return m_ownerState; }

  void set_owner_pid(std::optional<std::uint64_t> pid) { m_ownerPid = pid; }
  void set_owner_host(std::optional<std::string> host) { m_ownerHost = std::move(host); }
  void set_started_at(std::optional<std::chrono::system_clock::time_point> t) { m_startedAt = t; }
  void set_last_heartbeat_at(std::optional<std::chrono::system_clock::time_point> t) { m_lastHeartbeatAt = t; }
  void set_owner_state(std::optional<std::string> state) { m_ownerState = std::move(state); }

  /** Stream insertion operator
   * @param os The output stream
   * @param entry The entry to output
   * @return The output stream */
  friend std::ostream &operator<<(std::ostream &os, const MessageQueueEntry &entry);
};

}// namespace lintel
#endif// LINTEL_MESSAGEQUEUEENTRY_H
