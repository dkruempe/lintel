#include "base_library/features/base/repositories/MessageQueueRepository.h"

#include <base_library/core/configuration/ConnectionType.h>
#include <base_library/core/persistence/Connection.h>
#include <base_library/core/persistence/DatabaseConnectionConfigurations.h>
#include <base_library/core/persistence/ParameterBuilder.h>
#include <base_library/core/persistence/Transaction.h>
#include <base_library/core/persistence/PreparedStatement.h>
#include <base_library/core/configuration/MessageQueueEntry.h>
#include <cstdint>
#include <memory>
#include <vector>

import base_library.core.exceptions;

MessageQueueRepository::MessageQueueRepository(
  const std::shared_ptr<DatabaseConnectionConfigurations> &connectionConfigurations)
  : m_connectionConfigurations(connectionConfigurations),
    m_connectionEntry(connectionConfigurations->ofDefault()) {}

void MessageQueueRepository::insertOf(const std::vector<MessageQueueEntry> &entries)
{
  const db::Connection connection(m_connectionEntry);
  db::Transaction transaction(connection);
  db::PreparedStatement preparedStatement(connection,
    "insert into message_queues (name, process_name, max_messages) values (?, ?, ?)",
    "insert_message_queues");
  for (const auto &item : entries) {
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(item.get_message_queue_name());
    builder.add(item.get_process_name());
    builder.add(item.get_max_messages());
    preparedStatement.execute(builder);
  }
  transaction.commit();
}

void MessageQueueRepository::deleteOf(const std::vector<MessageQueueEntry> &entries)
{
  const db::Connection connection(m_connectionEntry);
  db::Transaction transaction(connection);
  db::PreparedStatement preparedStatement(connection,
    "delete from message_queues where name = ?",
    "delte_message_queues");
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
  std::string stmt;
  switch (m_connectionEntry->getType()) {
  case db::ConnectionType::SQLite:
    stmt = R"(select name,
                     process_name,
                     max_messages
             from message_queues
              where name REGEXP ?
               and  process_name REGEXP ?)";
    break;
  case db::ConnectionType::PostgreSQL:
    stmt = R"(select name,
                     process_name,
                     max_messages
              from message_queues
              where name ~* ?
              and process_name ~* ?)";
    break;
  default:
    throw db::SQLException("DatabaseType currently not supported");
  }
  const db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection, stmt, "message_queue_all_select");
  std::vector<MessageQueueEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(messageQueueName);
  builder.add(processName);
  auto result = preparedStatement.execute(builder);
  for (const auto &iter : result) {
    auto messageQueueEntry = MessageQueueEntry("",
      iter.of(1).getValue<std::string>(),
      iter.of(0).getValue<std::string>(),
      iter.of(2).getValue<int32_t>());
    entries.push_back(messageQueueEntry);
  }
  return entries;
}

std::vector<MessageQueueEntry> MessageQueueRepository::allProcessNameOf(const std::string &processName)
{
  std::string stmt = R"(select name,
                     process_name,
                     max_messages
             from message_queues
              where process_name = ?)";
  const db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection, stmt, "message_queue_all_select");
  std::vector<MessageQueueEntry> entries;
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(processName);
  auto result = preparedStatement.execute(builder);
  for (const auto &iter : result) {
    auto messageQueueEntry = MessageQueueEntry("",
      iter.of(1).getValue<std::string>(),
      iter.of(0).getValue<std::string>(),
      iter.of(2).getValue<int32_t>());
    entries.push_back(messageQueueEntry);
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
                     max_messages
              from message_queues
              where name REGEXP ?
              limit 1)";
    break;
  case db::ConnectionType::PostgreSQL:
    stmt = R"(select name,
                     process_name,
                     max_messages
              from message_queues
              where name ~* ?
              limit 1)";
    break;
  default:
    throw db::SQLException("DatabaseType currently not supported");
  }
  const db::Connection connection(m_connectionEntry);
  db::PreparedStatement preparedStatement(connection, stmt, "message_queues_select");
  db::ParameterBuilder builder(m_connectionEntry);
  builder.add(name);
  auto result = preparedStatement.execute(builder);
  if (result.getSize() <= 0) {
    throw db::SQLException("message queue with name pattern '" + name +
                           "' not found");
  }
  MessageQueueEntry messageQueueEntry("",
    result.of(0).of(1).getValue<std::string>(),
    result.of(0).of(0).getValue<std::string>(),
    result.of(0).of(2).getValue<int32_t>());
  return messageQueueEntry;
}