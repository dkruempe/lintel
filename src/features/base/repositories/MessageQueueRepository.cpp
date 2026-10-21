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

  /** Read column `attribute` of `row` as a string, or nullopt when the column
   *  is NULL / missing / holds an unexpected type. The persistence layer
   *  signals all three by throwing, so the failure is mapped to nullopt here
   *  instead of being swallowed at the call site. */
  std::optional<std::string> optionalStringColumn(db::Result &result, std::size_t row, std::size_t attribute)
  {
    if (result.of(row).size() <= attribute) { return std::nullopt; }
    try {
      std::string value = result.of(row).of(attribute).getValue<std::string>();
      if (value.empty()) { return std::nullopt; }
      return value;
    } catch (const std::exception &) {
      return std::nullopt;
    }
  }

  /** Read column `attribute` of `row` as an unsigned 64-bit value. The column
   *  may be stored either as text or as a native integer depending on the
   *  backend, so both representations are tried; any failure means nullopt. */
  std::optional<std::uint64_t> optionalUint64Column(db::Result &result, std::size_t row, std::size_t attribute)
  {
    if (const std::optional<std::string> text = optionalStringColumn(result, row, attribute)) {
      try {
        return static_cast<std::uint64_t>(std::stoull(*text));
      } catch (const std::exception &) {
        return std::nullopt;
      }
    }
    try {
      const std::int64_t value = result.of(row).of(attribute).getValue<std::int64_t>();
      if (value < 0) { return std::nullopt; }
      return static_cast<std::uint64_t>(value);
    } catch (const std::exception &) {
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

  // The owner columns were added after the table existed, so a row may either
  // predate them (fewer columns) or hold NULL. Both mean "no owner recorded";
  // the helpers map every such case to std::nullopt.
  MessageQueueOwnerColumns owner;
  if (result.of(row).size() > 3) {
    owner.pid = optionalUint64Column(result, row, 3);
    owner.host = optionalStringColumn(result, row, 4);
    if (const std::optional<std::string> started = optionalStringColumn(result, row, 5)) {
      owner.startedAt = parseTime(*started);
    }
    if (const std::optional<std::string> heartbeat = optionalStringColumn(result, row, 6)) {
      owner.lastHeartbeatAt = parseTime(*heartbeat);
    }
    owner.state = optionalStringColumn(result, row, 7);
  }
  return MessageQueueEntry("", processName, name, maxMessages, owner);
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


std::vector<MessageQueueEntry> MessageQueueRepository::allOf(const MessageQueueFilter &filter)
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
  builder.add(filter.queueName);
  builder.add(filter.processName);
  auto result = preparedStatement.execute(builder);
  for (std::size_t row = 0; row < static_cast<std::size_t>(result.getSize()); ++row) {
    entries.push_back(entryFromResult(result, row));
  }
  return entries;
}

Page<MessageQueueEntry> MessageQueueRepository::pageOf(const MessageQueueFilter &filter,
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
  builder.add(filter.queueName);
  builder.add(filter.processName);
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

void MessageQueueRepository::registerOwner(const MessageQueueOwnerRegistration &registration)
{
  if (m_connectionEntry == nullptr) { return; }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(scopedConnection.connection(),
    "update message_queues set owner_pid = ?, owner_host = ?, started_at = ?, last_heartbeat_at = ?, owner_state = ? "
    "where name = ?",
    "update_message_queue_owner");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(static_cast<std::int64_t>(registration.pid));
  builder.add(registration.hostName);
  builder.add(systemTimeToString(registration.startedAt));
  // the initial heartbeat is the registration itself - a later updateHeartbeat() refreshes it
  builder.add(systemTimeToString(registration.startedAt));
  builder.add(registration.state);
  builder.add(registration.queueName);
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

void MessageQueueRepository::updateOwnerState(const MessageQueueOwnerStateUpdate &update)
{
  if (m_connectionEntry == nullptr) { return; }
  db::ScopedConnection scopedConnection(m_connectionEntry);
  db::Transaction transaction(scopedConnection.connection());
  db::PreparedStatement preparedStatement(scopedConnection.connection(),
    "update message_queues set owner_state = ? where name = ?",
    "update_message_queue_owner_state");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(update.state);
  builder.add(update.queueName);
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
