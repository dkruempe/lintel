#include "lintel/features/base/repositories/MessageQueueRepository.h"

#include <cstdint>
#include <lintel/core/exceptions/SQLException.h>
#include <lintel/core/persistence/Connection.h>
#include <lintel/core/persistence/ConnectionType.h>
#include <lintel/core/persistence/DatabaseConnectionConfigurations.h>
#include <lintel/core/persistence/ParameterBuilder.h>
#include <lintel/core/persistence/PreparedStatement.h>
#include <lintel/core/persistence/ScopedConnection.h>
#include <lintel/core/persistence/Transaction.h>
#include <lintel/core/services/LoggerService.h>
#include <lintel/core/utils/DateTimeFormat.h>
#include <lintel/features/base/configuration/MessageQueueEntry.h>
#include <memory>
#include <vector>

namespace lintel {
namespace {
  std::string systemTimeToString(const std::chrono::system_clock::time_point &tp)
  {
    return datetime::formatSystemTime(std::chrono::time_point_cast<std::chrono::microseconds>(tp));
  }

  std::optional<std::chrono::system_clock::time_point> parseTime(const std::string &s)
  {
    if (s.empty()) { return std::nullopt; }
    try {
      auto t = datetime::parseSystemTime(s);
      return std::chrono::time_point_cast<std::chrono::system_clock::duration>(t);
    } catch (...) {
      return std::nullopt;
    }
  }
}// namespace

MessageQueueRepository::MessageQueueRepository(
  const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations)
  : m_connectionConfigurations(connectionConfigurations), m_connectionEntry(connectionConfigurations->ofDefault())
{}

MessageQueueEntry MessageQueueRepository::entryFromResult(db::Result &result, std::size_t row) const
{
  auto name = result.of(row).of(0).getValue<std::string>();
  auto processName = result.of(row).of(1).getValue<std::string>();
  auto maxMessages = result.of(row).of(2).getValue<int32_t>();
  std::optional<std::uint64_t> ownerPid;
  std::optional<std::string> ownerHost;
  std::optional<std::chrono::system_clock::time_point> startedAt;
  std::optional<std::chrono::system_clock::time_point> lastHeartbeatAt;
  std::optional<std::string> ownerState;
  try {
    if (result.of(row).size() > 3) {
      try {
        auto v = result.of(row).of(3).getValue<std::string>();
        if (!v.empty()) { ownerPid = static_cast<std::uint64_t>(std::stoull(v)); }
      } catch (...) {
        try {
          auto v = result.of(row).of(3).getValue<std::int64_t>();
          if (v >= 0) { ownerPid = static_cast<std::uint64_t>(v); }
        } catch (...) {}
      }
      try {
        auto v = result.of(row).of(4).getValue<std::string>();
        if (!v.empty()) { ownerHost = v; }
      } catch (...) {}
      try {
        auto v = result.of(row).of(5).getValue<std::string>();
        if (!v.empty()) { startedAt = parseTime(v); }
      } catch (...) {}
      try {
        auto v = result.of(row).of(6).getValue<std::string>();
        if (!v.empty()) { lastHeartbeatAt = parseTime(v); }
      } catch (...) {}
      try {
        auto v = result.of(row).of(7).getValue<std::string>();
        if (!v.empty()) { ownerState = v; }
      } catch (...) {}
    }
  } catch (...) {}
  return MessageQueueEntry(
    "", processName, name, maxMessages, ownerPid, ownerHost, startedAt, lastHeartbeatAt, ownerState);
}

void MessageQueueRepository::insertOf(const std::vector<MessageQueueEntry> &entries)
{
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  // Two statements instead of one with bound NULLs: the persistence layer binds
  // every parameter as text, so a NULL can only come from the column defaults.
  db::PreparedStatement plainStatement(scopedConnection.connection(),
    "insert into message_queues (name, process_name, max_messages) values (?, ?, ?)",
    "insert_message_queues");
  db::PreparedStatement ownerStatement(scopedConnection.connection(),
    "insert into message_queues (name, process_name, max_messages, owner_pid, owner_host, started_at, "
    "last_heartbeat_at, owner_state) values (?, ?, ?, ?, ?, ?, ?, ?)",
    "insert_message_queues_owner");
  for (const auto &item : entries) {
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(item.get_message_queue_name());
    builder.add(item.get_process_name());
    builder.add(item.get_max_messages());
    const bool hasOwner = item.get_owner_pid().has_value() || item.get_owner_host().has_value();
    if (!hasOwner) {
      plainStatement.execute(builder);
      continue;
    }
    builder.add(static_cast<std::int64_t>(item.get_owner_pid().value_or(0)));
    builder.add(item.get_owner_host().value_or(""));
    builder.add(systemTimeToString(item.get_started_at().value_or(std::chrono::system_clock::now())));
    builder.add(systemTimeToString(item.get_last_heartbeat_at().value_or(std::chrono::system_clock::now())));
    builder.add(item.get_owner_state().value_or("active"));
    ownerStatement.execute(builder);
  }
  transaction.commit();
}

void MessageQueueRepository::deleteOf(const std::vector<MessageQueueEntry> &entries)
{
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(
    scopedConnection.connection(), "delete from message_queues where name = ?", "delte_message_queues");
  for (const auto &item : entries) {
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(item.get_message_queue_name());
    preparedStatement.execute(builder);
  }
  transaction.commit();
}


std::vector<MessageQueueEntry> MessageQueueRepository::allOf(const std::string &processName,
  const std::string &messageQueueName)
{
  if (m_connectionEntry == nullptr) {
    LOG_INFO("no default database connection - skip message queue sync from database");
    return {};
  }
  std::string stmt;
  switch (m_connectionEntry->getType()) {
  case db::ConnectionType::SQLite:
    stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
             from message_queues
              where name REGEXP ?
               and  process_name REGEXP ?)";
    break;
  case db::ConnectionType::PostgreSQL:
    stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
              from message_queues
              where name ~* ?
              and process_name ~* ?)";
    break;
  default:
    throw db::SQLException("DatabaseType currently not supported");
  }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::PreparedStatement preparedStatement(scopedConnection.connection(), stmt, "message_queue_all_select");
  std::vector<MessageQueueEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(messageQueueName);
  builder.add(processName);
  auto result = preparedStatement.execute(builder);
  for (std::size_t row = 0; row < static_cast<std::size_t>(result.getSize()); ++row) {
    entries.push_back(entryFromResult(result, row));
  }
  return entries;
}

Page<MessageQueueEntry> MessageQueueRepository::pageOf(const std::string &processName,
  const std::string &messageQueueName,
  const std::optional<std::string> &afterName,
  std::size_t limit)
{
  if (limit == 0) { return Page<MessageQueueEntry>({}, false, std::nullopt); }
  if (m_connectionEntry == nullptr) {
    LOG_INFO("no default database connection - skip message queue sync from database");
    return Page<MessageQueueEntry>({}, false, std::nullopt);
  }
  std::string stmt;
  switch (m_connectionEntry->getType()) {
  case db::ConnectionType::SQLite:
    stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
             from message_queues
              where name REGEXP ?
               and  process_name REGEXP ?)";
    break;
  case db::ConnectionType::PostgreSQL:
    stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
              from message_queues
              where name ~* ?
              and process_name ~* ?)";
    break;
  default:
    throw db::SQLException("DatabaseType currently not supported");
  }
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(messageQueueName);
  builder.add(processName);
  if (afterName.has_value()) {
    stmt += " and name > ?";
    builder.add(afterName.value());
  }
  stmt += " order by name asc limit ?";
  builder.add(limit + 1);
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::PreparedStatement preparedStatement(scopedConnection.connection(), stmt, "message_queue_page_select");
  auto result = preparedStatement.execute(builder);
  std::vector<MessageQueueEntry> entries;
  const bool hasMore = static_cast<std::size_t>(result.getSize()) > limit;
  const std::size_t pageSize = hasMore ? limit : static_cast<std::size_t>(result.getSize());
  for (std::size_t i = 0; i < pageSize; i++) { entries.push_back(entryFromResult(result, i)); }
  std::optional<std::string> nextAfter = std::nullopt;
  if (hasMore && !entries.empty()) { nextAfter = entries.back().get_message_queue_name(); }
  return Page<MessageQueueEntry>(std::move(entries), hasMore, nextAfter);
}

std::vector<MessageQueueEntry> MessageQueueRepository::allProcessNameOf(const std::string &processName)
{
  std::string stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
             from message_queues
              where process_name = ?)";
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::PreparedStatement preparedStatement(scopedConnection.connection(), stmt, "message_queue_all_select");
  std::vector<MessageQueueEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(processName);
  auto result = preparedStatement.execute(builder);
  for (std::size_t row = 0; row < static_cast<std::size_t>(result.getSize()); ++row) {
    entries.push_back(entryFromResult(result, row));
  }
  return entries;
}

MessageQueueEntry MessageQueueRepository::allMessageQueueNameOf(const std::string &name)
{
  std::string stmt;
  switch (m_connectionEntry->getType()) {
  case db::ConnectionType::SQLite:
    stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
              from message_queues
              where name REGEXP ?
              limit 1)";
    break;
  case db::ConnectionType::PostgreSQL:
    stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
              from message_queues
              where name ~* ?
              limit 1)";
    break;
  default:
    throw db::SQLException("DatabaseType currently not supported");
  }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::PreparedStatement preparedStatement(scopedConnection.connection(), stmt, "message_queues_select");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(name);
  auto result = preparedStatement.execute(builder);
  if (result.getSize() <= 0) { throw db::SQLException("message queue with name pattern '" + name + "' not found"); }
  return entryFromResult(result, 0);
}

void MessageQueueRepository::registerOwner(const std::string &queueName,
  const std::string &processName,
  const std::string &hostName,
  std::uint64_t pid,
  const std::chrono::system_clock::time_point &startedAt,
  const std::string &ownerState)
{
  if (m_connectionEntry == nullptr) { return; }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(scopedConnection.connection(),
    "update message_queues set owner_pid = ?, owner_host = ?, started_at = ?, last_heartbeat_at = ?, owner_state = ? "
    "where name = ?",
    "update_message_queue_owner");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(static_cast<std::int64_t>(pid));
  builder.add(hostName);
  builder.add(systemTimeToString(startedAt));
  // the initial heartbeat is the registration itself - a later updateHeartbeat() refreshes it
  builder.add(systemTimeToString(startedAt));
  builder.add(ownerState);
  builder.add(queueName);
  preparedStatement.execute(builder);
  transaction.commit();
}

void MessageQueueRepository::updateHeartbeat(const std::string &queueName,
  const std::chrono::system_clock::time_point &heartbeatAt)
{
  if (m_connectionEntry == nullptr) { return; }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(scopedConnection.connection(),
    "update message_queues set last_heartbeat_at = ? where name = ?",
    "update_message_queue_heartbeat");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(systemTimeToString(heartbeatAt));
  builder.add(queueName);
  preparedStatement.execute(builder);
  transaction.commit();
}

void MessageQueueRepository::updateOwnerState(const std::string &queueName, const std::string &ownerState)
{
  if (m_connectionEntry == nullptr) { return; }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(scopedConnection.connection(),
    "update message_queues set owner_state = ? where name = ?",
    "update_message_queue_owner_state");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(ownerState);
  builder.add(queueName);
  preparedStatement.execute(builder);
  transaction.commit();
}

std::vector<MessageQueueEntry> MessageQueueRepository::findOrphans(const std::chrono::seconds &staleThreshold)
{
  std::vector<MessageQueueEntry> result;
  if (m_connectionEntry == nullptr) { return result; }
  const auto cutoff = std::chrono::system_clock::now() - staleThreshold;
  std::string stmt = R"(select name,
                     process_name,
                     max_messages,
                     owner_pid,
                     owner_host,
                     started_at,
                     last_heartbeat_at,
                     owner_state
             from message_queues
              where owner_pid is not null and (last_heartbeat_at is null or last_heartbeat_at < ?))";
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::PreparedStatement preparedStatement(scopedConnection.connection(), stmt, "find_orphan_message_queues");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(systemTimeToString(cutoff));
  auto res = preparedStatement.execute(builder);
  for (std::size_t row = 0; row < static_cast<std::size_t>(res.getSize()); ++row) {
    try {
      result.push_back(entryFromResult(res, row));
    } catch (...) {
      continue;
    }
  }
  return result;
}

void MessageQueueRepository::reclaimOrphans(const std::vector<MessageQueueEntry> &entries)
{
  if (m_connectionEntry == nullptr) { return; }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(scopedConnection.connection(),
    "update message_queues set owner_pid = null, owner_host = null, started_at = null, last_heartbeat_at = null, "
    "owner_state = null where name = ?",
    "reclaim_orphan_message_queues");
  for (const auto &item : entries) {
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(item.get_message_queue_name());
    preparedStatement.execute(builder);
  }
  transaction.commit();
}

void MessageQueueRepository::clearOwner(const std::string &queueName)
{
  if (m_connectionEntry == nullptr) { return; }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(scopedConnection.connection(),
    "update message_queues set owner_pid = null, owner_host = null, started_at = null, last_heartbeat_at = null, "
    "owner_state = null where name = ?",
    "clear_owner_message_queue");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(queueName);
  preparedStatement.execute(builder);
  transaction.commit();
}
}// namespace lintel
