#include "base_library/core/persistence/Transaction.h"
#include "base_library/features/base/repositories/HistoryRepository.h"
#include "base_library/features/base/models/HistoryEntry.h"
#include "base_library/core/persistence/DatabaseConnectionConfigurations.h"
#include "base_library/core/persistence/ParameterBuilder.h"
#include "base_library/core/persistence/Connection.h"
#include "base_library/core/exceptions/SQLException.h"
#include "base_library/core/services/LoggerService.h"

#include "base_library/core/persistence/PreparedStatement.h"

#include <chrono>
#include <memory>
#include <vector>

HistoryRepository::HistoryRepository(
        const std::shared_ptr<DatabaseConnectionConfigurations>
        &databaseConnectionConfigurations)
        : m_connectionConfigurations(databaseConnectionConfigurations),
          m_connectionEntry(m_connectionConfigurations->ofDefault()) {
}

std::vector<HistoryEntry> HistoryRepository::allOf() const {
    db::Connection const connection(m_connectionEntry);
    db::PreparedStatement preparedStatement(connection,
                                            R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history)",
                                            "history_all_of");
    std::vector<HistoryEntry> entries;
    const db::ParameterBuilder builder(m_connectionEntry);
    const auto result = preparedStatement.execute(builder);
    for (const auto &item: result) {
        const std::string processName = item.of(0).getValue();
        const std::string serviceName = item.of(1).getValue();
        const std::string label = item.of(2).getValue();
        const std::string text = item.of(3).getValue();
        auto createdTimestamp =
                item.of(4).getValue<date::sys_time<std::chrono::microseconds> >();
        const HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
        entries.push_back(entry);
    }
    return entries;
}

std::vector<HistoryEntry> HistoryRepository::allOf(
        const std::string &label) const {
    db::Connection const connection(m_connectionEntry);
    db::PreparedStatement preparedStatement(connection,
                                            R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where label = ?)",
                                            "history_all_label_of");
    std::vector<HistoryEntry> entries;
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(label);
    db::Result result = preparedStatement.execute(builder);
    for (auto &item: result) {
        const std::string processName = item.of(0).getValue();
        const std::string serviceName = item.of(1).getValue();
        const std::string text = item.of(3).getValue();
        auto createdTimestamp =
                item.of(4).getValue<date::sys_time<std::chrono::microseconds> >();
        const HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
        entries.push_back(entry);
    }
    return entries;
}

std::vector<HistoryEntry> HistoryRepository::allOf(const std::string &processName, const std::string &serviceName,
                                                   const std::string &label) const {
    std::string statement = "";
    switch (m_connectionEntry->getType()) {
        case db::ConnectionType::SQLite:
            statement = R"(
            select process_name,
                   service_name,
                   label,
                   text,
                   created_timestamp
            from history
            where process_name REGEXP ?
              and service_name REGEXP ?
              and label REGEXP ?)";
            break;
        case db::ConnectionType::PostgreSQL:
            statement = R"(
            select process_name,
                   service_name,
                   label,
                   text,
                   created_timestamp
            from history
            where process_name ~* ?
              and service_name ~* ?
              and label ~* ?)";
            break;
        default:
            throw db::SQLException("DatabaseType currently not supported");
    }
    db::Connection const connection(m_connectionEntry);
    db::PreparedStatement preparedStatement(connection, statement, "history_all_process_service_label_of");
    std::vector<HistoryEntry> entries;
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(processName);
    builder.add(serviceName);
    builder.add(label);
    db::Result result = preparedStatement.execute(builder);
    for (auto &item: result) {
        const std::string processNameTemp = item.of(0).getValue();
        const std::string serviceNameTemp = item.of(1).getValue();
        const std::string labelTemp = item.of(2).getValue();
        const std::string text = item.of(3).getValue();
        auto createdTimestamp =
                item.of(4).getValue<date::sys_time<std::chrono::microseconds> >();
        const HistoryEntry entry(processNameTemp, serviceNameTemp, labelTemp, text, createdTimestamp);
        entries.push_back(entry);
    }
    return entries;
}

std::vector<HistoryEntry> HistoryRepository::allOf(
        const std::string &processName, const std::string &serviceName) const {
    db::Connection const connection(m_connectionEntry);
    db::PreparedStatement preparedStatement(connection,
                                            R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where process_name = ?
        and service_name = ?)",
                                            "history_all_process_service_of");
    std::vector<HistoryEntry> entries;
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(processName);
    builder.add(serviceName);
    db::Result result = preparedStatement.execute(builder);
    for (auto &item: result) {
        const std::string label = item.of(2).getValue();
        const std::string text = item.of(3).getValue();
        auto createdTimestamp =
                item.of(4).getValue<date::sys_time<std::chrono::microseconds> >();
        const HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
        entries.push_back(entry);
    }
    return entries;
}

std::vector<HistoryEntry> HistoryRepository::allOfProcess(
        const std::string &processName) const {
    db::Connection const connection(m_connectionEntry);
    db::PreparedStatement preparedStatement(connection,
                                            R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where process_name = ?)",
                                            "history_all_process_of");
    std::vector<HistoryEntry> entries;
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(processName);
    db::Result result = preparedStatement.execute(builder);
    for (auto &item: result) {
        const std::string serviceName = item.of(1).getValue();
        const std::string label = item.of(2).getValue();
        const std::string text = item.of(3).getValue();
        auto createdTimestamp =
                item.of(4).getValue<date::sys_time<std::chrono::microseconds> >();
        const HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
        entries.push_back(entry);
    }
    return entries;
}

std::vector<HistoryEntry> HistoryRepository::allOfService(
        const std::string &serviceName) const {
    db::Connection const connection(m_connectionEntry);
    db::PreparedStatement preparedStatement(connection,
                                            R"(
       select process_name,
              service_name,
              label,
              text,
              created_timestamp
      from history
      where service_name = ?)",
                                            "history_all_service_of");
    std::vector<HistoryEntry> entries;
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(serviceName);
    db::Result result = preparedStatement.execute(builder);
    for (auto &item: result) {
        const std::string processName = item.of(0).getValue();
        const std::string label = item.of(2).getValue();
        const std::string text = item.of(3).getValue();
        auto createdTimestamp =
                item.of(4).getValue<date::sys_time<std::chrono::microseconds> >();
        const HistoryEntry entry(processName, serviceName, label, text, createdTimestamp);
        entries.push_back(entry);
    }
    return entries;
}

void HistoryRepository::insertOf(const std::vector<HistoryEntry> &entries) const {
    try {
        db::Connection const connection(m_connectionEntry);
        db::Transaction const transaction(connection);
        db::PreparedStatement preparedStatement(connection, R"(
    insert into history (
    process_name,
    service_name,
    label,
    text,
    uuid,
    created_timestamp)
    values (?, ?, ?, ?, ?, ?)
)", "history_insert_of");
        for (const auto &entry: entries) {
            db::ParameterBuilder builder(m_connectionEntry);
            builder.add(entry.getProcessName());
            builder.add(entry.getServiceName());
            builder.add(entry.getLabel());
            builder.add(entry.getText());
            builder.add(entry.getUuid());
            builder.add(entry.getCreatedTimestamp());
            preparedStatement.execute(builder);
        }
    } catch (db::SQLException &exception) {
        LOG_ERROR("insert failed: {}", exception.what());
    }
}

void HistoryRepository::cleanAllOlderThan(
        date::sys_time<std::chrono::microseconds> timestamp) const {
    const db::Connection connection(m_connectionEntry);
    const db::Transaction transaction(connection);
    db::PreparedStatement preparedStatement(connection, R"(
    delete from history
    where created_timestamp < ?
  )",
                                            "history_cleanup");
    db::ParameterBuilder builder(m_connectionEntry);
    builder.add(timestamp);
    preparedStatement.execute(builder);
}
